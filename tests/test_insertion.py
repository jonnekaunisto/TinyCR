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

def test_inserting():
    success = True
    output = ""

    total_certificates = 10000000 #10 million
    print(str(int(0.99*total_certificates)))
    print(str(int(0.01*total_certificates)))


    if enable_server_print:
        server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))])
    else:
        with open("server.out", "w") as f:
            server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))],
                                        stdout=f)
    while(True):
        time.sleep(1)
        try:
            data = send_to_server("ping")
            print(data)
            if data == "pong":
                break
        except:
            print("Not reachable")
        

    if enable_client_print:
        client_process = subprocess.Popen([client_path])
    else:
        with open("client.out", "w") as f:
            client_process = subprocess.Popen([client_path], stdout=f)

    time.sleep(3)
    try:
        print("inserting")
        for i in range(int(total_certificates) + 1, int(1.99 * total_certificates)):
            start = time.time()
            send_to_server("add {} 1".format(i))
            end = time.time()
            print(end - start)
            
            #time.sleep(0.5)
        for i in range(int(1.99 * total_certificates) + 1, int(2 * total_certificates)):
            send_to_server("add {} 0".format(i))
            #time.sleep(0.5)
        print("done inserting")

    except Exception as e:
        success = False
        print(e)
        server_process.kill()

    print("sleeping")
    time.sleep(13)
    send_to_server("exi")
    stdout,stderr = server_process.communicate()
    print(stdout)
    print(stderr)

    client_process.terminate()

    assert success


    