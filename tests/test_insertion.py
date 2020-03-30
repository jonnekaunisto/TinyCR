import pytest
import subprocess
import os
import socket
import time
import curses

src_dir_path = os.path.dirname(os.path.abspath(__file__)) + '/../src/build'
server_path = src_dir_path + '/server'
client_path = src_dir_path + '/client'

client_ip = "localhost"
client_port = 60000

server_ip = "localhost"
server_port = 50000

enable_server_print = False

enable_client_print = False

def send_to_server(message):
    #print("sending: " + message)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((server_ip, server_port))
        s.sendall(str.encode(message))
        data = s.recv(1024)
    data = str(data, encoding="utf-8")
    #print("received: " + data)
    return data

def send_to_client(message):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((client_ip, client_port))
        s.sendall(str.encode(message))
        data = s.recv(1024)
    data = str(data, encoding="utf-8")
    #print("received: " + data)
    return data

def wait_until_server_ready():
    while(True):
        time.sleep(1)
        try:
            data = send_to_server("ping")
            print(data)
            if data == "pong":
                break
        except:
            print("Not reachable")

def test_inserting():
    success = True
    output = ""

    total_init_certs = 1000000
    total_insert_certs = 1000
    print(str(int(0.01*total_init_certs)) + " " + str(int(0.99*total_init_certs)))

    if enable_server_print:
        server_process = subprocess.Popen([server_path, str(int(0.01*total_init_certs)), str(int(0.99*total_init_certs))])
    else:
        with open("server.out", "w") as f:
            server_process = subprocess.Popen([server_path, str(int(0.01*total_init_certs)), str(int(0.99*total_init_certs))],
                                        stdout=f)
    
    wait_until_server_ready() 

    if enable_client_print:
        client_process = subprocess.Popen([client_path])
    else:
        with open("client.out", "w") as f:
            client_process = subprocess.Popen([client_path], stdout=f)
    time.sleep(3)
    try:
        '''
        for i in range(int(0.01*total_init_certs)):
            response = send_to_client("show {}".format(i))
            if "is revoked" not in response:
                print(response)
                raise Exception("Not revoked when supposed to be revoked")

        for i in range(int(0.01*total_init_certs), total_init_certs):
            response = send_to_client("show {}".format(i))
            print(i)
            if "is unrevoked" not in response:
                print(response)
                raise Exception("Not unrevoked when supposed to be unrevoked")
        '''

        insert_pos_high = total_init_certs + 1 + int(0.99 * total_insert_certs)
        insert_pos_low = total_init_certs + 1
        print("inserting: {}:{}".format(insert_pos_low, insert_pos_high))
        for i in range(insert_pos_low, insert_pos_high):
            send_to_server("add {} 1".format(i))
            
        insert_neg_high = total_init_certs + 1 + total_insert_certs
        insert_neg_low = total_init_certs + 1 + int(0.99 * total_insert_certs) + 1
        print("inserting neg: {}:{}".format(insert_neg_low, insert_neg_high))
        for i in range(insert_neg_low, insert_neg_high):
            send_to_server("add {} 0".format(i))
            #time.sleep(0.5)
        print("done inserting")

        for i in range(int(0.01*total_init_certs)):
            response = send_to_client("show {}".format(i))
            print(i)
            if "is revoked" not in response:
                print(response)
                raise Exception("Not revoked when supposed to be revoked")

        for i in range(int(0.01*total_init_certs), total_init_certs):
            response = send_to_client("show {}".format(i))
            if "is unrevoked" not in response:
                print(response)
                raise Exception("Not unrevoked when supposed to be unrevoked")


    except Exception as e:
        success = False
        print(e)
        server_process.kill()
        client_process.kill()
        assert success

    print("sleeping")
    time.sleep(5)
    send_to_server("exi")
    stdout,stderr = server_process.communicate()

    client_process.terminate()

    assert success


    