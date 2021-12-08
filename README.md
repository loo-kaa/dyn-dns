# dyn-dns

Dynamic dns program written in c. I've tested it with valgrind to avoid memory leaks but it has yet to be tryed out in the real world for long periods of time.

If you'd like to use it on your server to automatically update a cloudflare dns record to your current ip start by cloning the repository.

Clone the repository in the `/usr/local/sbin/dyn-dns` directory.

Enter the local/sbin directory.
```sh
cd /usr/local/sbin
```

Clone the repository.
```sh
git clone https://github.com/luca-a/dyn-dns.git
```

## 1. Setup

Install libpthread, libcurl and libsystemd on your os. Below you can find some examples.

### Ubuntu

```sh
sudo apt install libcurl4 libpthread-stubs0-dev libsystemd-dev
```

## 2. Build

After having installed the required packages you can run make and build the executable.

```sh
make
```

## 3. Daemon configuration setup

Create the `/etc/dyn-dns` directory and then create the file `/etc/dyn-dns/cloudflare.config` by using the command:

```sh
mkdir /etc/dyn-dns/
cp sample.config /etc/dyn-dns/cloudflare.config
```

➡ **Edit the `/etc/dyn-dns/cloudflare.config` file and set the appropriate values** (retrieved from your cloudflare dashboard):

```sh
nano /etc/dyn-dns/cloudflare.config
```

## 4. Copy daemon unit files

At this point you just have to copy the service file to the system directory, reload the configs, enable and start the timer:

```sh
cp dyn-dns.service /etc/systemd/system

systemctl daemon-reload

systemctl enable dyn-dns.service
systemctl start dyn-dns.service
```

## 5. Create a crontab entry to signal the external ip check

Launch the following commands as root to create a crontab entry that will check the current ip and update it if needed every minute.

```sh
touch /etc/cron.d/dyn-dns
tee -a /etc/cron.d/dyn-dns << END
* * * * * root kill -USR1 \$(systemctl show --property MainPID --value dyn-dns.service)
END
```

Now you should have a working dynamic dns for the record id you configured in the **cloudflare.config** file.

> :information_source: **To launch the check and update of the configured dns record you just have to signal the process.**
> Using crontab is one way of doing it but you can signal it however you want.
