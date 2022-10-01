import asyncio
import websockets
import pyperclip

async def main():
    async with websockets.connect("ws://vinca_reader.local:81") as websocket:
        print("OK, Ready for input")
        while True:
            data = await websocket.recv()
            data = data.strip()
            data = data[1:] if '*' in data else data
            pyperclip.copy(data)


if __name__ == "__main__":
    asyncio.run(main())
