FROM debian:10

RUN apt-get update
RUN apt-get install -y gcc git wget make libncurses-dev flex bison gperf python python-serial python-pip

RUN cd /tmp && \
  wget https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz && \
  tar xzf xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz -C /opt
ENV PATH $PATH:/opt/xtensa-lx106-elf/bin

RUN git clone https://github.com/espressif/ESP8266_RTOS_SDK.git /third_party/ESP8266_RTOS_SDK && \
  cd /third_party/ESP8266_RTOS_SDK && \
  git checkout release/v3.3
RUN python -m pip install -r /third_party/ESP8266_RTOS_SDK/requirements.txt

ENV IDF_PATH /third_party/ESP8266_RTOS_SDK
