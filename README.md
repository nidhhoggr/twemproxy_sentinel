```
 _____                                          ___          _   _          _ 
|_   _|_ __ _____ _ __  _ __ _ _ _____ ___  _  / __| ___ _ _| |_(_)_ _  ___| |
  | | \ V  V / -_) '  \| '_ \ '_/ _ \ \ / || | \__ \/ -_) ' \  _| | ' \/ -_) |
  |_|  \_/\_/\___|_|_|_| .__/_| \___/_\_\\_, | |___/\___|_||_\__|_|_||_\___|_|
                       |_|               |__/                                 
```

#Twemproxy Sentinel

###Author: Joseph Persie

---

###Purpose

The purpose of this library is to be ran as a linux daemon to monitor a redis sentinel. The daemon will
subscribe to the sentinel and listen for master change events. This is the event that is fired when a redis
master fails over and the designated slave is promoted. The daemon will in turn update the nutcracker 
configuration file to use the new redis master IP address.

---

###Build
 
`make`

---

###Run
```
./build/twemproxy_sentinel -h [redis-sentinel-host] -p [redis_sentinel_port] \
  -f [nutcracker_config_file] -c [twemproxy_service_name]
```
