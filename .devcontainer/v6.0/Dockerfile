FROM espressif/idf:release-v6.0

USER root

RUN apt-get update && apt-get install -y sudo cppcheck && rm -rf /var/lib/apt/lists/*

RUN echo "root ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN echo ". /opt/esp/idf/export.sh" >> /root/.bashrc