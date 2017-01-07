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
```sh
./build/twemproxy_sentinel -h [redis-sentinel-host] -p [redis_sentinel_port] \
  -f [nutcracker_config_file] -c [twemproxy_service_name] [ [-n [sentinel_channel_name] ]
```

###Sample Output

```sh
[root@Startup-Twemproxy2 twemproxy_sentinel]# ./build/twemproxy_sentinel -h startup-redissentinel -p 26379 -f /root/twemproxy_sentinel/conf/nutcracker.yml -c twemproxy
TS ARGS:
 - ip: startup-redissentinel
 - port: 26379
 - twemproxy_config_path: /root/twemproxy_sentinel/conf/nutcracker.yml
 - twemproxy_service_name: twemproxy
 - log_file_location: (null)
 - sentinel channel name: +switch-master
master (0): 10.132.16.48:6383

master (1): 10.132.16.48:6379

master (2): 10.132.16.48:6380

master (3): 10.132.16.48:6382

master (4): 10.132.16.48:6381

MATCH: master name: redis-002, config name: redis-002
MATCH: master name: redis-003, config name: redis-003
MATCH: master name: redis-004, config name: redis-004
DOESNT MATCH: master uri: 10.132.16.48:6382,  config uri 10.132.169.170:6382
promoting master to 10.132.16.48:6382:1 redis-004
MATCH: master name: redis-005, config name: redis-005
MATCH: master name: redis-001, config name: redis-001
Redirecting to /bin/systemctl restart  twemproxy.service
twemproxy sentinel listenting to sentinel on channel: SUBSCRIBE +switch-master
0) subscribe
1) +switch-master
2) (null)
```
