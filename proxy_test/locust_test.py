# locust_test.py
# In python, '#' is used to indicate a comment line.
"""
The string within triple-quote is also considered as a comment.
And the triple-quote can be used for multiline comments.
DISCLAIMER: This sample doesn't care about whether the use of API is correct.
"""

import sys, random
from locust import HttpLocust, TaskSet

proxies = {
  'http': 'http://localhost:53176',
  'https': 'http://localhost:53176',
}

def getHome(locust):
    """ define a function in python whose name is getList and the argument is locust """
    locust.client.get('/home.html', proxies=proxies)
    # locust.client.get('/editor/post?action=list&username=cs144')

def getAdder(locust):
    """ define a function in python whose name is previewPage and the argument is locust """
    # postid = random.randint(1, 100) # generate a random number from 1 to 100 (include 1 and 100)
    # url_prefix = '/blog/cs144/';
    url_prefix = '/cgi-bin/adder?'
    Adder_a = random.randint(1, 10);
    Adder_b = random.randint(1, 10);
    locust.client.get(url_prefix + str(Adder_a) + '&' + str(Adder_b), name=url_prefix, proxies=proxies)

class MyTaskSet(TaskSet):
    """ the class MyTaskSet inherits from the class TaskSet, defining the behavior of the user """
    tasks = {getHome: 1, getAdder: 1}
    def on_start(locust):
        """ on_start is called when a Locust start before any task is scheduled """
        # response = locust.client.post("/login", data={"username":"cs144", "password": "password"})
        # if response.status_code != 200:
            # print("FAIL to start with posting data to server. Make sure that your server is running.")
            # sys.exit();
        pass

class MyLocust(HttpLocust):
    """ the class MyLocust inherits from the class HttpLocust, representing an HTTP user """
    task_set = MyTaskSet
    min_wait = 1000
    max_wait = 2000
