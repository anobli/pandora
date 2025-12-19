# Copyright (c) 2026 Alexandre Bailon
#
# SPDX-License-Identifier: Apache-2.0

import re
import pytest

def pytest_addoption(parser):
    parser.addoption('--server-ipv4')
    parser.addoption('--server-ipv6')

@pytest.fixture()
def ot_hardware_map(request):
    return request.config.getoption('--ot-hardware-map')

def ipv4_shell(shell):
    shell.wait_for_prompt()

    iface = shell.exec_command('net iface 1')
    filtered_iface = shell.get_filtered_output(iface)

    ipv4_list = []

    for line in filtered_iface:
        ip = re.findall(r'[0-9]+(?:\.[0-9]+){3}', line)
        if ip:
            ipv4_list.append(ip[0])

    return ipv4_list

@pytest.fixture()
def ipv4(request):
    ipv4_addr = request.config.getoption('--server-ipv4')
    if ipv4_addr:
        return ipv4_addr

    try:
        shell = request.getfixturevalue("shell")
        if shell:
            ipv4_addrs = ipv4_shell(shell)
            if ipv4_addrs:
                return ipv4_addrs[0]
    except pytest.FixtureLookupError:
        return None
    
    return None

def ipv6_shell(shell):
    shell.wait_for_prompt()

    iface = shell.exec_command('net iface 1')
    filtered_iface = shell.get_filtered_output(iface)

    ipv6_list = []

    for line in filtered_iface:            
        ip = re.findall(r'\b(?:[A-Fa-f0-9]{1,4}:){7}[A-Fa-f0-9]{1,4}\b', line)
        if ip:
            ipv6_list.append(ip)

    return ipv6_list

@pytest.fixture()
def ipv6(request):
    ipv6_addr = request.config.getoption('--server-ipv6')
    if ipv6_addr:
        return ipv6_addr

    try:
        shell = request.getfixturevalue("shell")
        if shell:
            ipv6_addrs = ipv6_shell(shell)
            if ipv6_addrs:
                return ipv6_addrs[0]
    except pytest.FixtureLookupError:
        return None

    return None

@pytest.fixture()
def ip(ipv4, ipv6):
    assert ipv4 is not None or ipv6 is not None

    if ipv4:
        return ipv4
    return ipv6
