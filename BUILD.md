# Manual Build/Install Instructions

This guide will help you in case you want to understand/build/install uTCPIPPublisher from scratch.

***

## STEP 1: Building

### Overall Pre-requisites:

* gcc-c++ or C++11 Compatible Compiler (like GCC >=5)
* SystemD enabled system (ubuntu, centos, fedora, etc)
* libMantids-devel
* pthread
* zlib-devel
* openssl-devel
* boost-devel

***

Having these prerequisites (eg. by yum install), you can start the build process (as root) by doing:

```
cd /root
git clone https://github.com/unmanarc/uTCPIPPublisher
cmake -B../builds/uTCPIPPublisher . -DCMAKE_VERBOSE_MAKEFILE=ON
cd ../builds/uTCPIPPublisher
make -j12 install
```

Now, the application is installed in the operating system, you can proceed to the next step

## STEP 2: Installing files and configs

Then:
- copy the **/etc/uTCPIPPublisher** directory

```
cp -a ~/uTCPIPPublisher/etc/uTCPIPPublisher /etc/
```


## STEP 3: Service Intialization

We have two services, the client and the server, you can copy them to the system and initialize them:

```
cp uTCPIPPublisher/usr/lib/systemd/system/utcpippubclient.service /usr/lib/systemd/system/utcpippubclient.service 
cp uTCPIPPublisher/usr/lib/systemd/system/utcpippubserver.service /usr/lib/systemd/system/utcpippubserver.service 
```

Now execute daemon-reload from systemd to adquire the new files:
```
systemctl daemon-reload
```

After this, configure /etc/uTCPIPPublisher configuration files, and you can enable each service by executing this:

``` 
systemctl enable --now utcpippubserver
systemctl enable --now utcpippubclient
```

Remember not to activate both services if you don't know what are you doing.


Now you are done!

Congrats.