#!/usr/bin/env python3
import requests, re, json, websockets
import asyncio, time

# Globals
initial_REST_call_url = "https://discordapp.com/api/v6/gateway/bot"
with open("token.txt", "r") as file:
    initial_REST_call_token = file.read().strip('\n\r\s')
initial_REST_call_auth_head = "Bot " + initial_REST_call_token
initial_REST_call_headers = {'Content-Type': 'application/json', 'Authorization': initial_REST_call_auth_head}

# Globals (unset)
socketurl = None

def ppj(jd):
    print(json.dumps(jd, indent=2))

def print_api_call_details():
    print("Url:", initial_REST_call_url)
    ppj(initial_REST_call_headers)

def populate_socket_url():
    resp = requests.get(initial_REST_call_url, headers=initial_REST_call_headers)
    resp = resp.text
    print('> HTTPS /gateway/bot response:', resp)

    rd = json.loads(resp)
    ppj(rd)
    """
    {
      "url": "wss://gateway.discord.gg/",
      "shards": 9,
      "session_start_limit": {
        "total": 1000,
        "remaining": 999,
        "reset_after": 14400000
      }
    }
    """
    global socketurl
    socketurl = rd["url"].rstrip('/') + "/?v=6&encoding=json"

def get_heartbeat_full():
    return {'op': 1, 'd': None}

def get_presence():
    return { "game": { "name": "skribble.io", "type": 0 }, "status": "dnd", "since": 91879201, "afk": False }

def get_identify_payload():
    import sys
    d = {}

    # Add token
    d['token'] = initial_REST_call_token
    
    # Create properties
    p = {
        '$os': sys.platform,
        '$browser': 'QueueBot',
        '$device': 'QueueBot',
        '$referrer': '',
        '$referring_domain': ''
    }    
    d['properties'] = p

    # Add presence
    # d['presence'] = get_presence()

    return d

async def respond(ws, state):
    while True:
        packet = await ws.recv()
        print('Received packet:')
        ppj(safe_j(packet))
        #opcode = d['op']
        #payload = d['d']
        #resp = {'d': None, 'op': None}
        # await ws.send(json.dumps(resp))
        d = safe_j(packet)
        if 'op' not in d or d['op'] == 11:
            return

atomic_s = None

async def send_heartbeat(ws):
    print('Sending heartbeat...')
    hbd = {'op': 1, 's': atomic_s, 'd': {}, 't': None}
    await ws.send(json.dumps(hbd))

async def heartbeat_sender(ws, state):
    while True:
        await asyncio.sleep(state['hb_interval_s'])
        await send_heartbeat(ws)

def check(j,k,v):
    return k in j and j[k] == v
def safe_j(js):
    try:
        return json.loads(js)
    except:
        print("Could not load into json:", js)
        return {}

async def opcode_10(state, packet):
    if not check(packet,'op', 10):
        # Something went wrong, abort
        print('Packet was not correct for opcode 10:')
        ppj(packet)
        return False
    print('Got opcode 10:')
    ppj(packet)
    payload = packet['d']
    state['hb_interval_ms'] = payload['heartbeat_interval']
    state['hb_interval_s'] = payload['heartbeat_interval'] / 1000
    return True

async def send_identify(ws, s):
    resp = {'op': 2, 'd': get_identify_payload()}
    print('Sending opcode 2 packet:')
    ppj(resp)
    await ws.send(json.dumps(resp))

async def start_bot(state):
    import os
    async with websockets.connect(socketurl, ssl=True) as websocket:
        state = {}
        if await opcode_10(state, safe_j(await websocket.recv())) == False:
            return
        # We have loaded our hello payload now to do the next connection step!
        # First let's send a heartbeat, you know, just in case
        # await send_heartbeat(websocket)
        # Now let's send our identification package
        await send_identify(ws=websocket, s=state)
        print('Attempting to receive ready package...')
        ready = safe_j(await websocket.recv())
        ppj(ready)
        if not check(ready,'op',0):
            print('Exiting due to not being ready.')
            return
        print('Starting heartbeater/message receiver!')
        heartbeater = asyncio.ensure_future(heartbeat_sender(ws=websocket, state=state))
        message_printer = asyncio.ensure_future(respond(ws=websocket, state=state))
        await asyncio.wait(
            [heartbeater, message_printer],
            return_when=asyncio.FIRST_COMPLETED,
        )

def run_bot():
    state = {}
    populate_socket_url()
    asyncio.get_event_loop().run_until_complete(start_bot(state))

if __name__ == '__main__':
    import sys
    args = sys.argv
    if 'info' in args:
        print_api_call_details()
    if 'go' in args:
        run_bot()
