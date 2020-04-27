import time
import socket
import random

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

client_ip = "172.16.238."
client_port = 60000

server_ip = "172.16.238.2"
server_port = 50000

total_init_certs = 10000000
total_insert_certs = 1000


def send_to_server(message):
    #print("sending: " + message)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((server_ip, server_port))
        s.sendall(str.encode(message))
        data = s.recv(1024)
    data = str(data, encoding="utf-8")
    #print("received: " + data)
    return data

def send_to_random_client(clients, message):
    ip = random.choice(clients)
    return send_to(ip, client_port, message)

def send_to(ip, port, message):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((ip, port))
        s.sendall(str.encode(message))
        data = s.recv(1024)
    data = str(data, encoding="utf-8")
    return data

def wait_until_server_ready():
    while(True):
        time.sleep(1)
        try:
            data = send_to_server("ping")
            if data == "pong":
                break
        except:
            print("Not reachable")

def discover_clients(num_clients):
    clients = []

    starting_ip = 4
    for i in range(starting_ip, num_clients + starting_ip):
        curr_ip = client_ip + str(i)
        reached = False

        for j in range(3):
            time.sleep(1)
            try:
                data = send_to(curr_ip, client_port, "ping")
                if data == "pong":
                    reached = True
                    break
            except socket.error as e:
                pass
        
        if reached:
            clients.append(curr_ip)
        else:
            break

    return clients



def run_docker_test():
    print("Running Docker Test", flush=True)
    print("-"*15)
    clients = discover_clients(10)

    print(clients, flush=True)

    wait_until_server_ready()

    print("Validating server certificates", end="", flush=True)
    for i in range(1, int(0.01*total_init_certs), 10000):
        response = send_to_server("show {}".format(i))
        if "is revoked" not in response:
            print(response)
            raise Exception("Not revoked when supposed to be revoked")

    for i in range(int(0.01*total_init_certs), total_init_certs, 10000):
        response = send_to_server("show {}".format(i))
        if "is unrevoked" not in response:
            print(response)
            raise Exception("Not unrevoked when supposed to be unrevoked")
    print(f"{bcolors.OKGREEN} Success{bcolors.ENDC}", flush=True)

    insert_pos_high = total_init_certs + 1 + int(0.99 * total_insert_certs)
    insert_pos_low = total_init_certs + 1
    print("Inserting positive certificates: {}:{}".format(insert_pos_low, insert_pos_high), end='', flush=True)
    for i in range(insert_pos_low, insert_pos_high):
        send_to_server("add {} 1".format(i))

    print(f"{bcolors.OKGREEN} Success{bcolors.ENDC}", flush=True)
        
    insert_neg_high = total_init_certs + 1 + total_insert_certs
    insert_neg_low = total_init_certs + 1 + int(0.99 * total_insert_certs) + 1
    print("Inserting negative certificates: {}:{}".format(insert_neg_low, insert_neg_high), end='', flush=True)
    for i in range(insert_neg_low, insert_neg_high):
        send_to_server("add {} 0".format(i))

    print(f"{bcolors.OKGREEN} Success{bcolors.ENDC}", flush=True)


    print("Validating on-device revoked certificates", end="", flush=True)
    for i in range(1, int(0.01*total_init_certs), 1000):
        response = send_to_random_client(clients, "show {}".format(i))
        if "is revoked" not in response:
            print(response)
            raise Exception("Not revoked when supposed to be revoked")
    print(f"{bcolors.OKGREEN} Success{bcolors.ENDC}", flush=True)

    print("Validating on-device unrevoked certificates", end="", flush=True)
    for i in range(int(0.01*total_init_certs), total_init_certs, 1000):
        response = send_to_random_client(clients, "show {}".format(i))
        if "is unrevoked" not in response:
            print(response)
            raise Exception("Not unrevoked when supposed to be unrevoked")
    print(f"{bcolors.OKGREEN} Success{bcolors.ENDC}", flush=True)

    

    



if __name__ == "__main__":
    run_docker_test()