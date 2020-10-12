# pg\_crash

If your database is too reliable - pg\_crash can kill it for you. pg\_crash is
an extension to PostgreSQL, which allows you to periodically or randomly crash
your database infrastructure by sending kill (or other) signals to your DB
processes and make them fail. It is ideal for HA and failover testing.

Works with Postgres >= 10.0.

# Installation

* Build the extension

```
PG_CONFIG=/usr/local/pgsql/bin/pg_config make
sudo PG_CONFIG=/usr/local/pgsql/bin/pg_config make install
```
NB! Replace /usr/local/pgsql/bin with your desired binary

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

### Developer Credits

Antonin Houska, Cybertec PostgreSQL International GmbH.
Visit our website: [www.cybertec-postgresql.com](https://www.cybertec-postgresql.com)

