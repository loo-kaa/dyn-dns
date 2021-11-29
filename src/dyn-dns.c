#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <signal.h>
#include <systemd/sd-daemon.h>

#include <curl/curl.h>

#include "lib/logger.h"
#include "utils.h"
#include "mlib.h"

#define CALL_TIMEOUT_SEC 5

#define EXT_IP_MAX_LENGTH 16
#define EXT_IP_QUERY_URL "http://api.ipify.org/?format=text"

#define CLOUDFLARE_DNS_UPDATE_METHOD "PATCH"
#define CLOUDFLARE_DNS_UPDATE_URL "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s"
#define CLOUDFLARE_AUTHORIZATION_HEADER "Authorization: Bearer %s"
#define CLOUDFLARE_CONTENT_TYPE_HEADER "Content-Type: application/json"
#define CLOUDFLARE_DNS_PATCH_DATA "{\"content\":\"%s\"}"

// remove compile time access variables and use a config file to load clouflare access variables
#define DYN_DNS_ETC "/etc/dyn-dns/"
#define ACCESS_CONFIG_FILE_PATH DYN_DNS_ETC "cloudflare.config"

#define DYN_DNS_VAR "/var/lib/dyn-dns/"
#define PREV_ADDRESS_FILE_PATH DYN_DNS_VAR "prev_address.dat"

#define CLOUDFLARE_MAX_TOKEN_SIZE 512
#define CLOUDFLARE_ID_SIZE 32

char token[CLOUDFLARE_MAX_TOKEN_SIZE + 1] = { 0 };
char zone_id[CLOUDFLARE_ID_SIZE + 1] = { 0 };
char record_id[CLOUDFLARE_ID_SIZE + 1] = { 0 };

bool load_config_variables(char* config_file_path) {
	FILE* config_file = fopen(config_file_path, "r");

	if(config_file != NULL) {
		struct property* properties = read_property_file(config_file);

		char* temp;
	
		temp = get_property_value(properties, "TOKEN");
		if(temp != NULL)
			strncpy(token, temp, CLOUDFLARE_MAX_TOKEN_SIZE);

		temp = get_property_value(properties, "ZONE_ID");
		if(temp != NULL)
			strncpy(zone_id, temp, CLOUDFLARE_ID_SIZE);

		temp = get_property_value(properties, "RECORD_ID");
		if(temp != NULL)
			strncpy(record_id, temp, CLOUDFLARE_ID_SIZE);

		free_properties(properties);
		fclose(config_file);

		return true;	
	}

	return false;
}

void setup_dir(char* dir) {
	if (stat(dir, &(struct stat){}) == -1) {
		if(mkdir(dir, 0700) != 0) {
			sd_notifyf(0, "STATUS=Failed to start up: Couldn't create directory '%s'", dir);
			exit(EX_NOPERM);
		}
	}
}

static inline CURLcode perform_call(CURL* curl, const char* url, size_t (*callback)(char*, size_t, size_t, void*)) {
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

	log_debug("Performing call to '%s'", url);
	CURLcode result = curl_easy_perform(curl);

	curl_easy_reset(curl);

	return result;
}

char* read_prev_address() {
	FILE* prev_address_file = fopen(PREV_ADDRESS_FILE_PATH, "r");
	char* prev_address;

	// if it can't be opened it does not exist and we set an empty prev_address
	if(prev_address_file != NULL) {
		prev_address = reg_ptr(read_file(prev_address_file), daemon_run);
		fclose(prev_address_file);
		prev_address_file = NULL;
	}
	else {
		prev_address = c_malloc_reg(1, daemon_run);
		prev_address[0] = 0;
	}

	return prev_address;
}

void write_prev_address(char* address) {
	FILE* prev_address_file = fopen(PREV_ADDRESS_FILE_PATH, "w");
	fputs(address, prev_address_file);
	fclose(prev_address_file);
}

// current address call
char current_address[EXT_IP_MAX_LENGTH + 1] = "";

// callback for external address curl call
size_t address_callback(char* buffer, size_t itemSize, size_t itemCount, void* userdata) {
	size_t size = itemSize * itemCount,
		max_size = size < EXT_IP_MAX_LENGTH ? size : EXT_IP_MAX_LENGTH;

	memcpy(current_address, buffer, max_size);
	current_address[max_size] = 0;

	return size;
}

char* query_current_address(CURL* curl) {
	CURLcode result = perform_call(curl, EXT_IP_QUERY_URL, address_callback);

	if(result != CURLE_OK) {
		log_error("Call to '" EXT_IP_QUERY_URL "' resulted in an error (curl result = '%s')", curl_easy_strerror(result));
		return NULL;
	}

	return current_address;
}

// cloudflare patch call
bool cloudflare_success = false;

// callback for cloudflare patch record call
size_t cloudflare_patch_callback(char* buffer, size_t itemSize, size_t itemCount, void* userdata) {
	size_t size = itemSize * itemCount;

	// if we still need to search for the success string
	if(!cloudflare_success) {
		char* success_location = strstr(buffer, "\"success\":");

		// if the success key string was found
		if(success_location != NULL) {
			cloudflare_success = strstr_block(success_location, "true", ',') != NULL;
		}
	}

	return size;
}

struct curl_slist* add_header(struct curl_slist* headersList, const char* headerString) {
	struct curl_slist* temp = curl_slist_append(headersList, headerString);

	if(temp == NULL) {
		log_error("An error occurred while allocating the curl_slist memory");
		curl_slist_free_all(headersList);
		exit(EX_OSERR);
	}

	return temp;
}

bool patch_cloudflare_record(CURL* curl) {
	char* post_data = reg_ptr(format_string(CLOUDFLARE_DNS_PATCH_DATA, current_address), daemon_run);

	log_debug("The request body is '%s'", post_data);

	struct curl_slist* headers = NULL;
	headers = add_header(headers, reg_ptr(format_string(CLOUDFLARE_AUTHORIZATION_HEADER, token), daemon_run));
	headers = add_header(headers, CLOUDFLARE_CONTENT_TYPE_HEADER);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, CLOUDFLARE_DNS_UPDATE_METHOD);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	CURLcode result = perform_call(curl, reg_ptr(format_string(CLOUDFLARE_DNS_UPDATE_URL, zone_id, record_id), daemon_run), cloudflare_patch_callback);

	curl_slist_free_all(headers);

	if(result != CURLE_OK || !cloudflare_success) {
		log_error("Error in the curl call to update the cloudflare record (curl result = '%s', cloudflare success = '%s')",
			curl_easy_strerror(result), cloudflare_success ? "true" : "false");

		return false;
	}

	return true;
}

void dyn_dns_run(CURL* curl) {
	// TODO check https://dev.to/noah11012/templates-in-c-5b94 cause maybe we could find something cool to do to generalize the fetching of a generic result

	char *prev_address = read_prev_address(),
		*current_address = query_current_address(curl);

	if(current_address == NULL) {
		log_error("Can't update record, the query function for the current_address failed");
		return;
	}

	log_debug("The previous ip is '%s' the retrieved ip is '%s'", prev_address, current_address);

	if(strcmp(prev_address, current_address)) {
		log_status("Ip changed from '%s' to '%s' patching cloudflare dns record", prev_address, current_address);

		if(patch_cloudflare_record(curl)) {
			log_status("Cloudflare record updated successfully");

			write_prev_address(current_address);
		}
	}
	else {
		log_debug("There is nothing to do");
	}

	// removing daemon_run variables
	c_free_scope(daemon_run);

	return;
}

int main() {
	logger_set_out_daemon();
	logger_set_log_level(LOG_MAX_LEVEL_ERROR_WARNING_STATUS_DEBUG);
	
	setup_dir(DYN_DNS_ETC);
	setup_dir(DYN_DNS_VAR);
	
	if(!load_config_variables(ACCESS_CONFIG_FILE_PATH)) {
		sd_notifyf(0, "STATUS=Failed to start up: No configuration file '%s'", ACCESS_CONFIG_FILE_PATH);
		exit(EX_CONFIG);
	}
	
	curl_global_init(CURL_GLOBAL_ALL);
	atexit(curl_global_cleanup); // register cleanup function

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, CALL_TIMEOUT_SEC);
	reg_ptr_func(curl, curl_easy_cleanup, application); // register curl variable for cleanup after application

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGHUP);
	sigaddset(&sigset, SIGUSR1);

	// blocking signals in the sigset to let sigwait handle them
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	sd_notify(0, "READY=1");
	log_status("The dyn-dns daemon successfully started up");

	int sig;

	while(sigwait(&sigset, &sig) == 0 && sig != SIGTERM) {
		if(sig == SIGHUP) {
			if(load_config_variables(ACCESS_CONFIG_FILE_PATH)) {
				log_status("Configurations reloaded");
			}
			else {
				log_error("Couldn't reload configurations, please check the file '" ACCESS_CONFIG_FILE_PATH "'");
			}
		}
		else if(sig == SIGUSR1) {
			dyn_dns_run(curl);
		}
	}

	log_status("Stopping with received signal: %d", sig);

	sd_notify(0, "STOPPING=1");
	exit(EXIT_SUCCESS);
}
