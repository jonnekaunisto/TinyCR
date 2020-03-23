import subprocess
import os
import socket
import time
import numpy as np

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
        with open("client_perf.out", "w") as f:
            client_process = subprocess.Popen([client_path], stdout=f)

    time.sleep(3)
    
    print("inserting positive keys")
    positive_insert_perf = np.array([])
    for i in range(int(total_certificates) + 1, int(total_certificates) + 1001):
        start = time.time()
        send_to_server("add {} 1".format(i))
        end = time.time()
        positive_insert_perf = np.append(positive_insert_perf, end - start)
        #time.sleep(0.5)
    print("done inserting positive keys")
    print(np.average(positive_insert_perf))
    np.savetxt('positive_insert_perf.csv', positive_insert_perf, delimiter=',')

    print("inserting negative keys")
    negative_insert_perf = np.array([])
    for i in range(int(total_certificates) + 1001, int(total_certificates) + 2002):
        start = time.time()
        send_to_server("add {} 0".format(i))
        end = time.time()
        negative_insert_perf = np.append(negative_insert_perf, end - start)
    print("done inserting negative keys")
    print(np.average(negative_insert_perf))
    np.savetxt('negative_insert_perf.csv', negative_insert_perf, delimiter=',')

    print("moving positive key to negative")
    positive_move_perf = np.array([])
    for i in range(int(total_certificates) + 1, int(total_certificates) + 1001):
        start = time.time()
        send_to_server("rev {}".format(i))
        end = time.time()
        positive_move_perf = np.append(positive_move_perf, end - start)

    print("done moving positive keys to negative")
    print(np.average(positive_move_perf))
    np.savetxt('positive_move_perf.csv', positive_move_perf, delimiter=',')

    print("moving negative keys to positive")
    negative_move_perf = np.array([])
    for i in range(int(total_certificates) + 1001, int(total_certificates) + 2002):
        start = time.time()
        send_to_server("unr {}".format(i))
        end = time.time()
        negative_move_perf = np.append(negative_move_perf, end - start)

    print("done moving negative keys to positive")
    print(np.average(negative_move_perf))
    np.savetxt('negative_move_perf.csv', negative_move_perf, delimiter=',')

    print("sleeping")
    time.sleep(13)
    send_to_server("exi")
    stdout,stderr = server_process.communicate()
    print(stdout)
    print(stderr)

    client_process.terminate()   


if __name__ == '__main__':
    main() 