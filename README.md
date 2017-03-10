# RunCached

Execute commands while caching their output for subsequent calls. 
Command output will be cached for <cacheperiod> seconds and "replayed" for 
any subsequent calls. Original exit status will also be emulated.

## Details
After cacheperiod has expired, command will be re-executed and a new result 
will be cached. 
Cache data is tied to the command and arguments executed and the 
path of the runcached executable. Cache results are stored in /tmp

You can use runcached to run resource-expensive commands multiple times, 
parsing different parts of its output each time. Those commands will be
run only once for each cacheperiod. 

Implementation is provided in 3 languages, python, C, BASH. Of course the BASH version is not really suggested but it works.


## Usage

### Python
runcached.py [-c cacheperiod] <command to execute with args>

### C
runcached [-c cacheperiod] <command to execute with args>

### Bash
runcached.sh  <command to execute with args>



## Examples


### Example 1:  Run the date command. Each time it executes prints the same date for 5 seconds.
runcached.py -c 5 date

### Example 2: Zabbix userparameter which can be called multiple times , but actually executes only once every 20 seconds. 
Query multiple parameters of mysql at the same time, without re-running the query.


```
#!

UserParameter=mysql.globalstatus[*],/usr/local/bin/runcached.py -c 20 /usr/bin/mysql -ANe \"show global status\"|egrep '$1\b'|awk '{print $ 2}'
```


And then define some items like so:

```
#!nolang
Item Name                      Item Key
--------------                  --------------
MySQL DELETES	 	mysql.globalstatus[Com_delete]
MySQL INSERTS	 	mysql.globalstatus[Com_insert]
MySQL UPDATES	 	mysql.globalstatus[Com_update]
MySQL CREATE TABLE	mysql.globalstatus[Com_create_table]
MySQL SELECTS	 	mysql.globalstatus[Com_select]
MySQL Uptime	 	mysql.globalstatus[Uptime]
MySQL ALTER TABLE	mysql.globalstatus[Com_alter_table]

E.g. for DELETE: 
Type: Numeric, 
Data Type: Decimal. 
Units: QPS
Store Value: Delta (Speed per second)
Show Value: As Is
```