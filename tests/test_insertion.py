import pytest
import subprocess
import os
import socket
import time
import curses

src_dir_path = os.path.dirname(os.path.abspath(__file__)) + '/../src'
server_path = src_dir_path + '/server'
client_path = src_dir_path + '/client'

client_ip = "localhost"
client_port = 60000

server_ip = "localhost"
server_port = 50000

enable_server_print = False

enable_client_print = True

def send_to_server(message):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((server_ip, server_port))
        s.sendall(str.encode(message))
        data = s.recv(1024)
    return data

def test_inserting():
    success = True
    output = ""

    total_certificates = 1000 #10 million
    print(str(int(0.99*total_certificates)))
    print(str(int(0.01*total_certificates)))


    if enable_server_print:
        server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))])
    else:
        with open("server.out", "w") as f:
            server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))],
                                        stdout=f)
    
    time.sleep(10)

    if enable_client_print:
        client_process = subprocess.Popen([client_path])
    else:
        client_process = subprocess.Popen([client_path], stdout=subprocess.PIPE)

    time.sleep(3)
    try:
        print("inserting")
        for i in range(int(total_certificates), int(1.99 * total_certificates)):
            send_to_server("add {} 1".format(i))
            time.sleep(0.5)
        for i in range(int(1.99 * total_certificates), int(2 * total_certificates)):
            send_to_server("add {} 0".format(i))
            time.sleep(0.5)
        print("done inserting")

    except Exception as e:
        success = False
        print(e)
        server_process.kill()


    
    send_to_server("exi")
    stdout,stderr = server_process.communicate()
    print(stdout)
    print(stderr)

    client_process.terminate()

    assert success


    