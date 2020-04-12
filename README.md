## A simple multi-thread proxy
### Author: Yufeng Yang

The project was inspired by the proxy lab of cmu 15213 and there are still a
lot of to-do things.

### Run
```
cd my_proxy
make all
./proxy <available port>
```

### Key Files
```
proxy.c
- sbuf.c
- mycache.c
- list.h
- csapp.c
```

### Design
![Aaron Swartz](https://github.com/yyf710670079/my_proxy/raw/master/img/proxy_design.jpg)

### Load Test

Download locust software (Python package)
```
pip3 install locust
```

Use locust
```
cd proxy_test
locust -f locust_test.py --host=http://localhost:<server port>
```
