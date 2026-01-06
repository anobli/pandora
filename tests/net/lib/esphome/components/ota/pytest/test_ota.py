from ipaddress import IPv4Address, IPv6Address, ip_address
import os
import time
import socket
import hashlib
import random

#from twister_harness import Shell

base_dir = os.path.abspath(os.path.dirname(__file__))

NONE_OP = 0
RECV_TEST_OP = 1
SEND_OP = 2
TEST_OP = 3
CONNECT_OP = 4
DISCONNECT_OP = 5
CUSTOM_OP = 6

ERROR_NONE = 0
ERROR_LEN = 1
ERROR_DATA = 2

# From ESPHOME OTA, must remain aligned
USE_OTA_VERSION = 2

OTA_RESPONSE_OK = 0
OTA_RESPONSE_HEADER_OK = 0x40
OTA_RESPONSE_AUTH_OK = 0x41
OTA_RESPONSE_UPDATE_PREPARE_OK = 0x42
OTA_RESPONSE_BIN_MD5_OK = 0x43
OTA_RESPONSE_RECEIVE_OK = 0x44
OTA_RESPONSE_UPDATE_END_OK = 0x45
OTA_RESPONSE_CHUNK_OK = 0x47

OTA_RESPONSE_ERROR_MAGIC = 0x80
OTA_RESPONSE_ERROR_UPDATE_PREPARE = 0x81
OTA_RESPONSE_ERROR_AUTH_INVALID = 0x82
OTA_RESPONSE_ERROR_WRITING_FLASH = 0x83
OTA_RESPONSE_ERROR_UPDATE_END = 0x84
OTA_RESPONSE_ERROR_INVALID_BOOTSTRAPPING = 0x85
OTA_RESPONSE_ERROR_WRONG_CURRENT_FLASH_CONFIG = 0x86
OTA_RESPONSE_ERROR_WRONG_NEW_FLASH_CONFIG = 0x87
OTA_RESPONSE_ERROR_ESP8266_NOT_ENOUGH_SPACE = 0x88
OTA_RESPONSE_ERROR_ESP32_NOT_ENOUGH_SPACE = 0x89
OTA_RESPONSE_ERROR_NO_UPDATE_PARTITION = 0x8A
OTA_RESPONSE_ERROR_MD5_MISMATCH = 0x8B
OTA_RESPONSE_ERROR_UNKNOWN = 0xFF

MAGIC_BYTES = [0x6C, 0x26, 0xF7, 0x5C, 0x45]

FEATURE_SUPPORTS_COMPRESSION = 0x01

UPLOAD_BLOCK_SIZE = 8192
UPLOAD_BUFFER_SIZE = UPLOAD_BLOCK_SIZE * 8

class OTATestFile:
    def __init__(self, filename):
        self.contents = None
        with open(filename, "rb") as file_handle:
            self.contents = file_handle.read()
    
    def size(self):
        return len(self.contents)
    
    def md5(self):
        return hashlib.md5(self.contents).hexdigest()

class OTATestBufferEntry:
    def __init__(self, operation, data=None, data_len=0):
        self.operation = operation
        self.__data = data
        self.__len = data_len
        if data_len == 0 and data:
            self.__len = len(data)
        self.__error = ERROR_NONE

    def set_error(self, error):
        self.__error = error

    def get_error(self):
        return self.__error

    def custom_operation(self, sock):
        pass

    def corrupt_data(self, data, data_len):
        new_byte = random.randint(0, 255)
        new_index = random.randrange(0, data_len)
        new_data = bytearray(data)
        # TODO: check that value is not the same
        new_data[new_index] = new_byte
        return bytes(new_data)

    @property
    def data(self):
        if self.__error == ERROR_LEN:
            return self.__data[0:-1]
        elif self.__error == ERROR_DATA:
            return self.corrupt_data(self.__data, self.__len)
        else:
            return self.__data

    @property
    def len(self):
        if self.__error == ERROR_LEN:
            return self.__len - 1
        else:
            return self.__len

class OTAConnect(OTATestBufferEntry):
    def __init__(self, ip):
        super().__init__(CONNECT_OP, ip)

class OTADisconnect(OTATestBufferEntry):
    def __init__(self):
        super().__init__(DISCONNECT_OP)

class OTASendMagic(OTATestBufferEntry):
    def __init__(self):
        super().__init__(SEND_OP, bytes(MAGIC_BYTES))

class OTARecvVersion(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_OK, USE_OTA_VERSION]))

class OTASendFeatures(OTATestBufferEntry):
    def __init__(self):
        super().__init__(SEND_OP, bytes([FEATURE_SUPPORTS_COMPRESSION]))

class OTARecvAck(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_HEADER_OK]))

class OTARecvAuthOK(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_AUTH_OK]))

class OTASendSize(OTATestBufferEntry):
    def __init__(self, file):
        upload_size = file.size()
        upload_size_encoded = [
            (upload_size >> 24) & 0xFF,
            (upload_size >> 16) & 0xFF,
            (upload_size >> 8) & 0xFF,
            (upload_size >> 0) & 0xFF,
        ]
        super().__init__(SEND_OP, bytes(upload_size_encoded))

class OTARecvPrepareOK(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_UPDATE_PREPARE_OK]))

class OTASendMd5(OTATestBufferEntry):
    def __init__(self, file):
        super().__init__(SEND_OP, file.md5().encode("utf-8"))

class OTARecvMd5OK(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_BIN_MD5_OK]))

class OTASendData(OTATestBufferEntry):
    def __init__(self, file):
        super().__init__(CUSTOM_OP)
        self.file = file

    # TODO: Instead of using custom operation, we should build a list of send
    #       and receive operations. This should make more straigh forward
    #       testing with error conditions.
    def custom_operation(self, sock):
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 0)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, UPLOAD_BUFFER_SIZE)
        offset = 0
        while True:
            chunk = self.file.contents[offset : offset + UPLOAD_BLOCK_SIZE]
            if not chunk:
                break
            offset += len(chunk)

            # simulate corrupted data transfer if we want to test error handling
            if self.get_error() == ERROR_DATA:
                print("Corrupting data")
                chunk = self.corrupt_data(chunk, len(chunk))

            sock.sendall(chunk)
            # Our ESPHOME OTA server only support version 2
            # Don't try to support version 1
            data = sock.recv(1)
            assert data == bytes([OTA_RESPONSE_CHUNK_OK])

class OTARecvReceiveOK(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_RECEIVE_OK]))

class OTARecvUpdateEndOK(OTATestBufferEntry):
    def __init__(self):
        super().__init__(RECV_TEST_OP, bytes([OTA_RESPONSE_UPDATE_END_OK]))

class OTARecvError(OTATestBufferEntry):
    def __init__(self, error):
        super().__init__(RECV_TEST_OP, bytes([error]))

class OTASendAck(OTATestBufferEntry):
    def __init__(self):
        super().__init__(SEND_OP, bytes([OTA_RESPONSE_OK]))

class OTATestProtocol:
    def __init__(self, ip, file):
        self.sock = None
        self.entries = [
            OTAConnect(ip),
            OTASendMagic(),
            OTARecvVersion(),
            OTASendFeatures(),
            # Compression is not supported, just reply with a Ack
            OTARecvAck(),
            # Auth is not supported, just reply with Auth OK
            OTARecvAuthOK(),
            OTASendSize(file),
            OTARecvPrepareOK(),
            OTASendMd5(file),
            OTARecvMd5OK(),
            OTASendData(file),
            OTARecvReceiveOK(),
            OTARecvUpdateEndOK(),
            OTASendAck(),
            OTADisconnect(),
        ]

    def connect(self, ip_addr):
        # Until the server is ready
        # TODO: Found a more reliable way to figure it out
        self.wait(1)
        ip = ip_address(ip_addr)
        if isinstance(ip, IPv4Address):
            self.sock = socket.socket()
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 5)
            self.sock.connect((ip_addr, 8266))
        elif isinstance(ip, IPv6Address):
            self.sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0)
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 5)
            self.sock.connect((ip_addr, 8266))
        self.sock.settimeout(3)

    def close(self):
        self.sock.close()
        self.sock = None

    def wait(self, delay):
        time.sleep(delay)

    def __run(self, entries):
        for i, entry in enumerate(entries):
            print(f"\n=== Step {i}: {type(entry).__name__} ===")
            
            try:
                if entry.operation == CONNECT_OP:
                    print("Connecting")
                    self.connect(entry.data)
                    time.sleep(0.1)  # Small delay after connect
                    print("Connected successfully")

                elif entry.operation == DISCONNECT_OP:
                    print("Disconnecting")
                    self.close()
                    self.wait(1)

                elif entry.operation == SEND_OP:
                    print(f"Sending {entry.len} bytes: {entry.data.hex()}")
                    self.sock.sendall(entry.data)
                    print("Send complete")
                    time.sleep(0.05)  # Small delay to let data flush

                elif entry.operation == RECV_TEST_OP:
                    print(f"Expecting to receive {entry.len} bytes: {entry.data.hex()}")
                    data = self.sock.recv(entry.len)
                    print(f"Received {len(data)} bytes: {data.hex()}")
                    assert len(data) == entry.len, f"Length mismatch: expected {entry.len}, got {len(data)}"
                    assert data == entry.data, f"Data mismatch: expected {entry.data.hex()}, got {data.hex()}"
                    print("Receive validated successfully")
                
                elif entry.operation == CUSTOM_OP:
                    print("Custom OP")
                    entry.custom_operation(self.sock)
                    
            except socket.timeout as e:
                print(f"TIMEOUT at step {i}: {type(entry).__name__}")
                print(f"Error: {e}")
                raise
            except AssertionError as e:
                print(f"ASSERTION FAILED at step {i}: {type(entry).__name__}")
                print(f"Error: {e}")
                raise
            except Exception as e:
                print(f"ERROR at step {i}: {type(entry).__name__}")
                print(f"Error: {e}")
                raise

    def __reset_error(self):
        for entry in self.entries:
            entry.set_error(ERROR_NONE)

    def run(self):
        self.__reset_error()
        self.__run(self.entries)

    def run_with_error(self, error_step, error_type, error_code):
        self.__reset_error()
        tmp = []
        for entry in self.entries:
            print(f"{type(entry)} == {error_step}")
            if type(entry) is not error_step:
                tmp.append(entry)
            else:
                entry.set_error(error_type)
                tmp.append(entry)
                if error_code:
                    tmp.append(OTARecvError(error_code))
                tmp.append(OTADisconnect())
                print(tmp)
                self.__run(tmp)
                break

ota_test_file = OTATestFile(base_dir + "/assets/native_sim/zephyr.signed.bin")

def test_magic(ip):
    test_protocol = OTATestProtocol(ip, ota_test_file)
    test_protocol.run_with_error(OTASendMagic, ERROR_LEN, OTA_RESPONSE_ERROR_MAGIC)
    test_protocol.run_with_error(OTASendMagic, ERROR_DATA, OTA_RESPONSE_ERROR_MAGIC)

def test_sample(ipv4, ipv6):
    if ipv4:
        test_protocol = OTATestProtocol(ipv4, ota_test_file)
        test_protocol.run()

    if ipv6:
        test_protocol = OTATestProtocol(ipv6, ota_test_file)
        test_protocol.run()

def test_send_corrupt_and_check_md5(ip):
    test_protocol = OTATestProtocol(ip, ota_test_file)
    test_protocol.run_with_error(OTASendData, ERROR_DATA, OTA_RESPONSE_ERROR_MD5_MISMATCH)
