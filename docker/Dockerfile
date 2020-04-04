FROM debian:10

RUN apt-get update
RUN apt-get install -y gcc git wget make libncurses-dev flex bison gperf python python-serial python-pip

RUN cd /tmp && \
  wget https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz && \
  tar xzf xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz -C /opt
ENV PATH $PATH:/opt/xtensa-lx106-elf/bin

ENV IDF_PATH /third_party/ESP8266_RTOS_SDK

RUN git clone https://github.com/espressif/ESP8266_RTOS_SDK.git $IDF_PATH && \
  cd $IDF_PATH && \
  git checkout release/v3.3
RUN python -m pip install -r $IDF_PATH/requirements.txt

RUN cd /tmp && \
  cp -r $IDF_PATH/examples/get-started/hello_world . && \
  cd hello_world && \
  make defconfig && \
  make
