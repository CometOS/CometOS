import sys
import traceback
import subprocess
import os
import time
import traceback
import datetime 

### iotlab imports
import iotlabcli 
import iotlabcli.experiment
import json
import util

def _getRessourceFromNodeList(nodelist, firmwareFile):
    ''' Transform node information read from json file to the CLI REST API
        format needed by the cli classes'''
    tmp = {'type':'physical', 'nodes': [elem['nwkAddr'] for elem in nodelist], 'firmware':firmwareFile, 'profile':None}
    return tmp


def submit_experiment(nodefiles, name, durationMinutes, startTime=None, fakeSubmit=False):
    ''' submits an experiment at the iotlab (or just produces the JSON string which would
        be used; if fakeSubmit==True;
        
        Keyword arguments:
        nodesfiles      - list of tuples containing a json node file and a corresponding firmwareFile
        name            - name of the experiment
        durationMinutes - duration of the experiment in minutes
        startTime       - unix timestamp to start experiment, UTC
        fakeSubmit      - instead of submitting, just print the created JSON string
    '''

    ### directly use REST-API to access IoT-Lab experiment submission, waiting etc.
    user, passwd = iotlabcli.auth.get_user_credentials()

    # print "user={0} passwd={1}".format(user, passwd)
    api = iotlabcli.rest.Api(user, passwd)

    if len(sys.argv) > 1:
        prefix = sys.argv[1]

    resources = []
    for pair in nodefiles:
        nodeList = util.parseNodeFile(pair[0])
        if nodeList != []:
            resources.append(_getRessourceFromNodeList(nodeList, pair[1]))

    result = iotlabcli.experiment.submit_experiment(api, name, durationMinutes, resources, startTime, fakeSubmit)
    if fakeSubmit:
        print iotlabcli.helpers.json_dumps(result)
        expid = None
    else:
        print "result: \n {0} \n is of type {1}".format(result, type(result))
        if type(result) == dict:
            expid = result['id']
        elif type(result) == str:
            jsonRes = json.loads(result)
            expid = int(jsonRes['id'])

    return expid


def wait_experiment(expid):
    ### directly use REST-API to access IoT-Lab experiment submission, waiting etc.
    user, passwd = iotlabcli.auth.get_user_credentials()

    # print "user={0} passwd={1}".format(user, passwd)
    api = iotlabcli.rest.Api(user, passwd)


    print "Waiting for experiment with ID {0} to start........".format(expid)
 
    result = iotlabcli.experiment.wait_experiment(api, expid)



