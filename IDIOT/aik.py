#!/usr/bin/python
'''
    Dylan Wright - dylan.wright@uky.edu
    EE480 - Assignment 2: The Making Of An IDIOT
    aik.py : Automatic interface to aik cgi
    Version:
        02-14-2016 : initial
        03-02-2016 : added check before printing to stderr
'''
import requests
import sys

AIKPATH="http://super.ece.engr.uky.edu:8088/cgi-bin/aik.cgi"
spec_file="IDIOT_spec"
prog_file="prog0.idiot"

def main():
    #spec_file = input("spec: ")
    prog_file = input("")
    sinput, iinput = input_files(spec_file, prog_file)
    response = post_inputs(sinput, iinput)

    msg_dict = parse_response(response.text)
    ''' 
    if (msg_dict["spec"] != "" and msg_dict["code"] != "" and msg_dict["anal"] != ""):
        print("Spec:", msg_dict["spec"], file=sys.stderr, sep="\n")
        print("Code:", msg_dict["code"], file=sys.stderr, sep="\n")
        print("Anal:", msg_dict["anal"], file=sys.stderr, sep="\n")
    '''
    print(msg_dict["text"], file=sys.stdout)
    print(msg_dict["data"], file=sys.stderr)

def input_files(spec_file, prog_file):
    file_in = open(spec_file, "r")
    sinput = file_in.read()
    file_in.close()
    file_in = open(prog_file, "r")
    iinput = file_in.read()
    file_in.close()
    return sinput, iinput

def post_inputs(sinput, iinput):
    payload = {"haveinput":"1", "sinput":sinput, "iinput":iinput}
    r = requests.post(AIKPATH, data=payload)
    return r

def parse_response(response_text):
    delim_start = "<PRE>"
    delim_end = "</PRE>"
    keys = ["spec", "code", "anal", "text", "data"]
    messages = []
    while (response_text.find(delim_start) != -1):
        start = response_text.find(delim_start) + len(delim_start)
        end = response_text.find(delim_end)
        msg = response_text[start:end].strip()
        response_text = response_text[end+len(delim_end):]
        messages.append(msg)
    msg_dict = dict(zip(keys, messages))
    return msg_dict

main()
