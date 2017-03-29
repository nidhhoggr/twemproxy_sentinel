```
 _____                                          ___          _   _          _ 
|_   _|_ __ _____ _ __  _ __ _ _ _____ ___  _  / __| ___ _ _| |_(_)_ _  ___| |
  | | \ V  V / -_) '  \| '_ \ '_/ _ \ \ / || | \__ \/ -_) ' \  _| | ' \/ -_) |
  |_|  \_/\_/\___|_|_|_| .__/_| \___/_\_\\_, | |___/\___|_||_\__|_|_||_\___|_|
                       |_|               |__/                                 
```

# Twemproxy Sentinel

### Author: Joseph Persie

---

### Purpose

The purpose of this program is to be ran as a linux daemon to monitor a redis sentinel. The daemon will
subscribe to the sentinel and listen for master change events. This is the event that is fired when a redis
master fails over and the designated slave is promoted. The daemon will in turn update the nutcracker 
configuration file to use the new redis master IP address.

---

### Build
 
`make`

---

### Run
```sh
./build/twemproxy_sentinel -h [redis-sentinel-host] -p [redis_sentinel_port] \
  -f [nutcracker_config_file] -c [twemproxy_service_name] [ [-n [sentinel_channel_name] ] \
  [ [-l [syslog_identifier] ]
```

### Sample Output

```sh
[root@Startup-Twemproxy2 twemproxy_sentinel]# ./build/twemproxy_sentinel -h startup-redissentinel -p 26379 -f /root/twemproxy_sentinel/conf/nutcracker.yml -c twemproxy
Redirecting to /bin/systemctl restart  twemproxy.service
```

```
[root@Startup-Twemproxy2 twemproxy_sentinel]# journalctl - SYSLOG_IDENTIFIER=twemproxy-sentinel -f
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
Retarted Twemproxy
twemproxy sentinel listenting to sentinel on channel: SUBSCRIBE +switch-master
0) subscribe
1) +switch-master
2) (null)
```

### Example Output Of Socket Reconnection
```
Mar 28 22:19:59 startup-twemproxy2 twemproxy-sentinel[20288]: 0) subscribe
Mar 28 22:19:59 startup-twemproxy2 twemproxy-sentinel[20288]: 1) +switch-master
Mar 28 22:19:59 startup-twemproxy2 twemproxy-sentinel[20288]: 2) (null)
Mar 28 22:20:23 startup-twemproxy2 twemproxy-sentinel[20288]: Attempting to recconect to redis sentinel with 4 retries remaining
Mar 28 22:20:45 startup-twemproxy2 twemproxy-sentinel[20288]: Attempting to recconect to redis sentinel with 3 retries remaining
Mar 28 22:21:06 startup-twemproxy2 twemproxy-sentinel[20288]: Attempting to recconect to redis sentinel with 2 retries remaining
Mar 28 22:21:28 startup-twemproxy2 twemproxy-sentinel[20288]: Attempting to recconect to redis sentinel with 1 retries remaining
Mar 28 22:21:28 startup-twemproxy2 twemproxy-sentinel[20288]: Was successfully able to reconnect
Mar 28 22:21:28 startup-twemproxy2 twemproxy-sentinel[20507]: master (0): 10.132.169.170:6380
Mar 28 22:21:28 startup-twemproxy2 twemproxy-sentinel[20507]: master (1): 10.132.169.170:6381
Mar 28 22:21:28 startup-twemproxy2 twemproxy-sentinel[20507]: master (2): 10.132.169.170:6382
```
