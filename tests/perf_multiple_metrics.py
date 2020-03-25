import subprocess
import os
import socket
import time
import numpy as np
import re

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

def send_all(lower_bound, upper_bound, command):
    for i in range(lower_bound, upper_bound):
        response = send_to_server(command.format(i))

def send_server_statistic_request(statistic):
    response = send_to_server("get {}".format(statistic))
    duration = re.findall("[0-9]+.[0-9]+", response)[0]
    return duration

def send_client_statistic_request(statistic):
    response = send_to_client("get {}".format(statistic))
    duration = re.findall("[0-9]+.[0-9]+", response)[0]
    return duration


def main():
    success = True
    output = ""

    total_certificates = 10000000 #10 million
    print(str(int(0.99*total_certificates)))
    print(str(int(0.01*total_certificates)))


    if enable_server_print:
        server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))])
    else:
        with open("server_perf.out", "w") as f:
            server_process = subprocess.Popen([server_path, str(int(0.99*total_certificates)), str(int(0.01*total_certificates))],
                                        stdout=f)
    retry = 1
    while(True):
        time.sleep(1)
        try:
            data = send_to_server("ping")
            print(data)
            if data == "pong":
                break
        except Exception as e:
            print(e)
            print("Not reachable, retry #" + str(retry))
            retry += 1
    print("Reached Starting Performance Check")
        

    if enable_client_print:
        client_process = subprocess.Popen([client_path])
    else:
        with open("client_perf.out", "w") as f:
            client_process = subprocess.Popen([client_path], stdout=f)

    time.sleep(3)
    
    try:
        print("inserting positive keys")
        send_all(int(total_certificates) + 1, int(total_certificates) + 1001, "add {} 1")

        print("inserting negative keys")
        send_all(int(total_certificates) + 1001, int(total_certificates) + 2002, "add {} 0")

        print("moving positive key to negative")
        send_all(int(total_certificates) + 1, int(total_certificates) + 1001, "rev {}")

        print("moving negative keys to positive")
        send_all(int(total_certificates) + 1001, int(total_certificates) + 2002, "unr {}")

    except Exception as e:
        print(e)

    server_statistics = ["calc_latency", "full_encoding_latency", "delta_encoding_latency", "overall_RTT"]
    client_statistics = ["full_updating_latency", "delta_updating_latency"]

    print("Server Metrics")
    for statistic in server_statistics:
        duration = send_server_statistic_request(statistic)
        print(statistic)
        print(duration)

    print("Client metrics")
    for statistic in client_statistics:
        duration = send_client_statistic_request(statistic)
        print(statistic)
        print(duration)


    print("sleeping")
    time.sleep(13)
    send_to_server("exi")
    stdout,stderr = server_process.communicate()
    #print(stdout)
    #print(stderr)

    client_process.terminate()   


if __name__ == '__main__':
    main() 