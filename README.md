# pg\_crash

A Postgres extension to send kill (or other) signal after some interval. Useful for testing HA failover scenarios.

Works with Postges 10.0.

# Installation

* Build the extension

```
PG_CONFIG=/usr/local/pgsql/bin/pg_config make
sudo PG_CONFIG=/usr/local/pgsql/bin/pg_config make install
```
NB! Replace /usr/local/pgsql/bin with your wished binary

* Edit server config

Add 'pg\_crash to shared\_preload\_libraries and configure signals and timeouts


```
shared_preload_libraries = ',pg_crash'
# any POSIX signals you want to emit from the background worker
crash.signals = '1 2 3'
# set delay (in seconds) between sending signals
crash.delay = 30
```

* restart the server 


## Have fun troubleshooting!

