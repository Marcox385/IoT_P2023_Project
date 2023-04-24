#!/bin/bash

apt update -y
apt upgrade -y
apt dist upgrade -y
apt install -y mosquitto mosquitto-clients
systemctl enable mosquitto.service
mosquitto -v
sudo mosquitto_passwd -c /etc/mosquitto/passwd iot
sed -i '1s/^/per_listener_settings true\n/' /etc/mosquitto/mosquitto.conf
echo "allow_anonymous false" >> /etc/mosquitto/mosquitto.conf
echo "listener 1883" >> /etc/mosquitto/mosquitto.conf
echo "password_file /etc/mosquitto/passwd" >> /etc/mosquitto/mosquitto.conf
systemctl restart mosquitto
systemctl status mosquitto
apt install nginx -y
systemctl enable nginx
systemctl status nignx
snap install core
snap refresh core
apt remove certbot
snap install --classic certbot
ln -s /snap/bin/certbot /usr/bin/certbot
systemctl stop nginx
certbot certonly --standalone
systemctl start nginx
apt install python3-pip

ufw allow 22
ufw allow 80
ufw allow 443
ufw allow 1883
ufw enable
ufw status

#/etc/letsencrypt/live/iotmadnessproject.hopto.org/fullchain.pem
#/etc/letsencrypt/live/iotmadnessproject.hopto.org/privkey.pem

#mosquitto_sub -h localhost -t testTopic -u iot -P iot