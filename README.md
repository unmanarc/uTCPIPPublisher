# uTCPIPPublisher 

Unmanarc's TCP/IP Local Port Publisher
  
Author: Aaron Mizrachi (unmanarc) <<aaron@unmanarc.com>>   
Main License: LGPLv3


***
## Installing packages (HOWTO)


- [Manual build guide](BUILD.md)
- COPR Packages (Fedora/CentOS/RHEL/etc):  
[![Copr build status](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/uTCPIPPublisher/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/amizrachi/unmanarc/package/uTCPIPPublisher/)


### Simple installation guide for Fedora/RHEL:

- Proceed to activate our repo's and download/install uTCPIPPublisher:

RHEL7:
```bash
# Install EPEL Repo
yum -y install epel-release
yum -y install yum-plugin-copr

# Install unmanarc's copr
yum copr enable amizrachi/unmanarc -y
yum -y install uTCPIPPublisher
```

RHEL8:
```bash
# Install EPEL Repo
dnf -y install 'dnf-command(config-manager)'
dnf config-manager --set-enabled powertools
dnf -y install epel-release

# Install unmanarc's copr
dnf copr enable amizrachi/unmanarc -y
dnf -y install uTCPIPPublisher
```


- Once installed, you can continue by activating/enabling the service as client:
```bash
systemctl enable --now utcpippubclient
```

or as server:
```bash
systemctl enable --now utcpippubserver
```


- Don't forget to open the firewall in the server:

```bash
# Add Website port:
firewall-cmd --zone=public --permanent --add-port 12389/tcp
# Reload Firewall rules
firewall-cmd --reload
```

Also, you may want to open the firewall for listenning services...


***
## Usage/HOWTO:




### Server Mode

For server mode, first configure `/etc/uTCPIPPublisher/server_configuration.ini`  and `/etc/uTCPIPPublisher/services.json`

in `/etc/uTCPIPPublisher/server_configuration.ini` you will find something like this:

```ini
# uTCPIPPublisher Default Configuration

[Logs]
Syslog=false
ShowColors=true
ShowDate=true
Debug=true

[Server]
Port=12389
Host=0.0.0.0
PSK=AABBCCDDEEFF00112233
Services=services.json
NoConnectionInPoolTimeoutMSecs=10000
GCPeriodSecs=20
```


In the server mode, the `Logs` section control the output of the application, the `Server` section control the listening main receptor for clients

- The `Port` will be the listening TCP/IP port for receiving the connection from the client, this is the one that will be ciphered with a weak PSK stream cipher. The encryption security should only rely on the underlying service encryption (eg. SSH/HTTPS/etc..)

- `Host` will be the listening IPv4 address on the server machine to receive the connections from the clients, 0.0.0.0 will listen on any interface, and it should be reachable from the client network.

- `PSK` will be used by any of the clients to encrypt the communication with the server. Warning: if the PSK is leaked/compromised, an attacker would be able to eavesdrop the underlying connection. however, if the connection itself is properly encrypted (eg. HTTPS), the data integrity and confidentiality should be kept protected.

- `NoConnectionInPoolTimeoutMSecs` is the wait time in milli seconds for a connection to abort if no uplink is found

- `GCPeriodSecs` is the wait time between garbage collection process, the garbage collector will try to ping every connection to determine if is available or not, if not, the connection is removed from the pool.

- The service file (json) should be configured as follows:

```json
[
        {
            "serviceName": "Remote HTTP Port",
            "serviceAuthKey" : "PSK1122334455667788",
            "listenAddr" : "0.0.0.0",
            "listenPort" : 10080
        },
        {
            "serviceName": "Remote SSH Port",
            "serviceAuthKey" : "PSK2233445566778899",
            "listenAddr" : "0.0.0.0",
            "listenPort" : 10022
        }
]
```

- You can configure as many ports as you want
- The `serviceName` and `serviceAuthKey`  will be the authentication logon for the service
- `serviceAuthKey` is also a relevant PSK, it should be unique on each service, don't repeat them, if you fail to protect this value, and with the .ini PSK, anyone can publish you a service on that port, and anyone can also connect to your published port in your private infrastructure using a mitm attack.
- `listenAddr` will listen on that address in your server computer
- `listenPort` will listen on that port in your server computer, if you introduce 0 (zero), it will listen at any available port.
- IMPORTANT: to avoid any potential data leak, don't forget to keep all the configuration files with RW------- permission (chmod 600).


### Client Mode

For client mode, first configure `/etc/uTCPIPPublisher/client_configuration.ini` 


the configuration is like the following one:

```ini
# uTCPIPPublisher Default Configuration

[Logs]
Syslog=false
ShowColors=true
ShowDate=true
Debug=true

[Server]
Port=12389
PSK=AABBCCDDEEFF00112233
Host=127.0.0.1
PoolSize=10

[Client]
ServiceName=Remote SSH Port
ServiceAuthKey=PSK2233445566778899
LocalHostAddr=127.0.0.1
LocalPort=22
```

In the client mode, the `Logs` section control the output of the application, the `Server` section control the communication with the server (address of the server to connect to, pre shared key, host, and pool size), and the `Client` section will request to map+authenticate the service with the server.

- The preshared (`PSK`) key will be used to cipher (without perfect forward secrecy) the communication, this is only a measure to obfuscate the protocol itself, we strongly recommend to use TLS encrypted communication in the underlying communication.

- The Pool Size (`PoolSize`) is used to define how many connections will be done to the main server, this is useful for having high availability in the server side, each time a connection is used, we need to send another one, having a pool will improve the performance and availability of the exposed service.

-In the `Client` section, you will describe the service name and authorization key (service level authentication), and the exposed host (can be in localhost, or anywhere in your network)

***

### TODO:

- Define multiple PSK per server



***

### Overall Pre-requisites:

* libMantids
* openssl
* cmake3
* C++11 Compatible Compiler (like GCC >=5)
