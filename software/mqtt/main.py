#!/usr/bin/env python3

"""A MQTT to InfluxDB Bridge

This script receives MQTT data and saves those to InfluxDB.

"""

import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

INFLUXDB_ADDRESS = '192.168.178.202'
INFLUXDB_USER = 'root'
INFLUXDB_PASSWORD = 'root'
INFLUXDB_DATABASE = 'home_db'

MQTT_ADDRESS = '192.168.178.202'
MQTT_USER = ''
MQTT_PASSWORD = ''
MQTT_TOPIC = 'trifasipower'  # [bme280|mijia]/[temperature|humidity|battery|status]
MQTT_REGEX = 'home/([^/]+)/([^/]+)'
MQTT_CLIENT_ID = 'MQTTInfluxDBBridge'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, 8086, INFLUXDB_USER, INFLUXDB_PASSWORD, None)


class SensorData(NamedTuple):
    location: str
    measurement: str
    avrms: float
    bvrms: float
    cvrms: float
    airms: float
    birms: float
    cirms: float
    nirms: float
    ahz: float
    bhz: float
    chz: float


def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)


def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    sensor_data = _parse_mqtt_message(msg.topic, msg.payload.decode('utf-8'))
    print(sensor_data)
    if sensor_data is not None:
        _send_sensor_data_to_influxdb(sensor_data)


def _parse_mqtt_message(topic, payload):
#    print(topic, payload.split(";"))
    match = payload.split(";")
    location = "Kuchl"
    measurement = "Trifasi"
    return SensorData(location, measurement, float(match[0]), float(match[1]), float(match[2]), float(match[3]), float(match[4]), float(match[5]), float(match[6]), float(match[7]), float(match[8]), float(match[9]))


def _send_sensor_data_to_influxdb(sensor_data):
    json_body = [
        {
            'measurement': sensor_data.measurement,
            'tags': {
                'location': sensor_data.location
            },
            'fields': {
                'avrms': sensor_data.avrms,
                'bvrms': sensor_data.bvrms,
                'cvrms': sensor_data.cvrms,
                'airms': sensor_data.airms,
                'birms': sensor_data.birms,
                'cirms': sensor_data.cirms,
                'nirms': sensor_data.nirms,
                'ahz': sensor_data.ahz,
                'bhz': sensor_data.bhz,
                'chz': sensor_data.chz
            }
        }
    ]
    print("yolo")
    influxdb_client.write_points(json_body)


def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)


def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTT_ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge')
    main()
