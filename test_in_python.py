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
    socketurl = rd["url"].rstrip('/') + "/?v=6&encoding=json"

def get_heartbeat_full():
    return {'op': 1, 'd': None}

def get_presence():
    return { "game": { "name": "skribble.io", "type": 0 }, "status": "dnd", "since": 91879201, "afk": False }

def get_identify_payload():
    d = {}

    # Add token
    d['token'] = token
    
    # Create properties
    p = {}
    p['os'] = 'macOS'
    p['browser'] = 'QueueBot.Test'
    p['device'] = 'QueueBot.Test'
    d['properties'] = p

    # Add presence
    d['presence'] = get_presence()

    return d

async def respond(ws, d, state):
    opcode = d['op']
    payload = d['d']
    resp = {'d': None, 'op': None}
    if opcode == 10:
        # hello payload
        state['hbi'] = payload['heartbeat_interval']
        # need to send our own hb back... eventually
        state['lbt'] = None # figure this out after, w/e
        # okay anyways let's identify ourselves ezpz righto
        resp['op'] = 2
        resp['d'] = get_identify_payload() 
        resp['s'] = d['s']
         
    await ws.send(json.dumps(resp))

async def oneheartbeat():
    import os
    async with websockets.connect(socketurl, ssl=True) as websocket:
        _weird = 0
        _state = {}
        while True:
            # This is literally the worst thing I have ever done
            # But it works, technically!
            if os.path.isfile("stop.rm") or _weird > 5:
                break
            # Check to see if we're getting a message over the socket
            rec = await websocket.recv()
            try:
                rec = json.loads(rec)
            except:
                # what happened? I don't know
                print('Could not load into json:', rec)
                _weird += 1
                continue
            await respond(ws=websocket, d=rec, state=_state)

def run_bot():
    asyncio.get_event_loop().run_until_complete(oneheartbeat())

