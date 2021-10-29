

from ogn.client import AprsClient
from ogn.parser import parse, ParseError
import re


pattern_table = re.compile(r'[N][ ]{1}')

def process_beacon(raw_message):
    try:
        beacon = parse(raw_message)
        messages = re.findall(pattern_table, beacon['raw_message'])
        if messages:
            print(messages)
        print(beacon['raw_message'])

    except ParseError as e:
        print('Error, {}'.format(e.message))

client = AprsClient(aprs_user='N0CALL')
client.connect()

try:
    client.run(callback=process_beacon, autoreconnect=True)
except KeyboardInterrupt:
    print('\nStop ogn gateway')
    client.disconnect()
