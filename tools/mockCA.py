import sys
import socket

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
    if len(sys.argv) > 1:
        server_ip = sys.argv[1]
    while(True):
        print("Command: ", end=' ')
        command = input()

        data = send_to_server(command)
        print("Received: " + data)


if __name__ == '__main__':
    main()