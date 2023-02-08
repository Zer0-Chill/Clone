#!/usr/bin/env python3

from flipper.app import App
from serial.tools.list_ports_common import ListPortInfo

import logging
import os
import tempfile
import subprocess
import serial.tools.list_ports as list_ports


class Main(App):
    def init(self):
        self.parser.add_argument("-p", "--port", help="CDC Port", default="auto")
        self.parser.set_defaults(func=self.update)

        # logging
        self.logger = logging.getLogger()

    def find_wifi_board(self):
        # idk why, but python thinks that list_ports.grep returns tuple[str, str, str]
        ports: list[ListPortInfo] = list(list_ports.grep("ESP32-S2"))  # type: ignore

        if len(ports) == 0:
            # Blackmagic probe serial port not found, will be handled later
            pass
        elif len(ports) > 1:
            raise Exception("More than one WiFi board found")
        else:
            port = ports[0]
            if os.name == "nt":
                port.device = f"\\\\.\\{port.device}"
            return port.device

    def download_latest(self, dir: str):
        import requests

        # TODO: get latest version
        urls = [
            "https://update.flipperzero.one/builds/blackmagic-firmware/zlo/dap-link/flash.command",
            "https://update.flipperzero.one/builds/blackmagic-firmware/zlo/dap-link/blackmagic.bin",
            "https://update.flipperzero.one/builds/blackmagic-firmware/zlo/dap-link/bootloader.bin",
            "https://update.flipperzero.one/builds/blackmagic-firmware/zlo/dap-link/partition-table.bin",
        ]

        if not os.path.exists(dir):
            self.logger.info(f"Creating directory {dir}")
            os.makedirs(dir)

        for url in urls:
            file_name = url.split("/")[-1]
            file_path = os.path.join(dir, file_name)
            self.logger.info(f"Downloading {url} to {file_path}")
            with open(file_path, "wb") as f:
                response = requests.get(url)
                f.write(response.content)

    def update(self):
        try:
            port = self.find_wifi_board()
        except Exception as e:
            self.logger.error(f"{e}")
            return 1

        if self.args.port != "auto":
            port = self.args.port

            available_ports = [p[0] for p in list(list_ports.comports())]
            if port not in available_ports:
                self.logger.error(f"Port {port} not found")
                return 1

        if port is None:
            self.logger.error("WiFi board not found")
            self.logger.info(
                "Please connect WiFi board to your computer, hold down BOOT button and press RESET button"
            )
            return 1

        # TODO: get real temporary dir
        dir = tempfile.TemporaryDirectory()
        dir_name = dir.name

        self.download_latest(dir_name)

        with open(os.path.join(dir_name, "flash.command"), "r") as f:
            flash_command = f.read()

        flash_command = flash_command.replace("\n", "").replace("\r", "")
        flash_command = flash_command.replace("(PORT)", port)

        args = flash_command.split(" ")[0:]
        args = list(filter(None, args))

        esptool_params = []
        esptool_params.extend(args)

        self.logger.info(f'Running command: "{" ".join(args)}" in "{dir_name}"')

        process = subprocess.Popen(
            esptool_params,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=dir_name,
            bufsize=1,
            universal_newlines=True,
        )

        # esptool.py will set returncode to 1, even if flashing was successful
        # because it will not be able to reset the board after flashing via usb
        success_counter = 0
        success_marker = "Hash of data verified."

        success_but_reset_failed = False

        while process.poll() is None:
            if process.stdout is not None:
                for line in process.stdout:
                    text = line.strip()
                    if text == success_marker:
                        success_counter += 1

                    if "can not exit the download mode over USB" in text:
                        success_but_reset_failed = True

                    self.logger.debug(f"{text}")

        dir.cleanup()

        if success_counter < 3:
            self.logger.error(f"Failed to flash WiFi board")
            return 1

        self.logger.info("WiFi board flashed successfully")

        if success_but_reset_failed:
            self.logger.info("Press RESET button on WiFi board to start it")

        return 0


if __name__ == "__main__":
    Main()()
