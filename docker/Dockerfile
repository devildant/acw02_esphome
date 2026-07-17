ARG PYTHON_VERSION

FROM python:${PYTHON_VERSION}-slim

ARG ESPHOME_VERSION

RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl unzip libusb-1.0-0 gcc g++ make libffi-dev libssl-dev && \
    rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir esphome==${ESPHOME_VERSION} esptool

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

CMD ["sleep", "infinity"]