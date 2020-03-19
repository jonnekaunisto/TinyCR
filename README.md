# TinyCR

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


## Server Interface

| Command Name | Description                                                                                                           | Arg 1                    | Arg 2                 |
|--------------|-----------------------------------------------------------------------------------------------------------------------|--------------------------|-----------------------|
| add          |  Adds a key with a certain value. | Key number to be added.  | The value of the key. |
| rem          | Removes a key.                                                                                                        | Key number to be removed | N/A                   |
| unr          | Unrevoke a key. Meaning flipping value from 0 to 1.                                                                   | Key to be unrevoked.     | N/A                   |
| rev          | Revoke a key. Meaning flipping value from 1 to 0.                                                                     | Key to be revoked.       | N/A                   |
| exi          | Exits the server safely.                                                                                              | N/A                      | N/A                   |

## Client Interface
| Command Name | Description                                   | Arg 1           |
|--------------|-----------------------------------------------|-----------------|
| show         | Queries the value in associated with the key. | The key number. |
