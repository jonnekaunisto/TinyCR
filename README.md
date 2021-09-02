# TinyCR

## Running Tests

Running tests requires docker

```bash
docker-compose up --scale client=5
```

## Testing out the system interactively

Running interactively with multiple clients requires docker

```bash
docker-compose up --scale test-framework=0 client=5
docker-compose logs
```

Running with single client can be done without docker

```bash
server &
client &
```

Communicate with server using the mockCA

```bash
python mockCA.py
```

## Usage

Go to source directory and make build dir

```bash
cd src
mkdir build
cd build
```

Run cmake

```bash
cmake -G "Unix Makefiles" ..
```

To compile the binaries.

```bash
make
```

To run the server

```bash
./server
```

To run the client.

```bash
./client
```

To communicate with the server.

```bash
./mockCA
```

To communicate with the client.

```bash
./mockCA localhost 60000
```

## Server Socket Interface

| Command Name | Description                                                                                                           | Arg 1                    | Arg 2                 |
|--------------|-----------------------------------------------------------------------------------------------------------------------|--------------------------|-----------------------|
| add          |  Adds a key with a certain value. | Key number to be added.  | The value of the key. |
| rem          | Removes a key.                                                                                                        | Key number to be removed | N/A                   |
| unr          | Unrevoke a key. Meaning flipping value from 0 to 1.                                                                   | Key to be unrevoked.     | N/A                   |
| rev          | Revoke a key. Meaning flipping value from 1 to 0.                                                                     | Key to be revoked.       | N/A                   |
| exi          | Exits the server safely.                                                                                              | N/A                      | N/A                   |

## Client Socket Interface

| Command Name | Description                                   | Arg 1           |
|--------------|-----------------------------------------------|-----------------|
| show         | Queries the value in associated with the key. | The key number. |


## License and Citation
This demo software is knowingly designed to illustrate technique(s) intended to defeat a system's security. It is open-sourced and free-to-use for research purposes only. The technical details of the design can be found at:

@inproceedings{shi2021tinycr,

    title={{On-device IoT Certificate Revocation Checking with Small Memory and Low Latency}},
  
    author={Shi, Xiaofeng and Shi, Shouqian and Wang, Minmei and Kaunisto, Jonne and Qian, Chen},
  
    booktitle={Proceedings of ACM CCS},
  
    year={2021}
  
}

