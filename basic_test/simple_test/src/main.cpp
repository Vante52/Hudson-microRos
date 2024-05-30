#include <Arduino.h>
#include <micro_ros_arduino.h>
#include <Adafruit_NeoPixel.h>
#include <std_msgs/msg/int32.h>
#include <rclc/executor.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

// Definir los pines
// Sensor de proximidad
const int trigPin = 13; // D7
const int echoPin = 15; // D10

// WS2812 LEDs
#define LED_PIN     0   // D8
#define LED_COUNT   8   // Número de LEDs en la tira

// Motores
// Motor 1
const int ENA =  1; // D1
const int IN1 =  16; // D2
const int IN2 =  5;  // D3

// Motor 2
const int ENB =  12; // D6
const int IN3 =  4;  // D4
const int IN4 =  14; // D5

// Definir variables
long duration;
int distance;
int safetyDistance;

// Crear un objeto para la tira de LED
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

rcl_publisher_t publisher;
std_msgs__msg__Int32 distance_msg;
rclc_support_t support;
rcl_node_t node;

void read_sensor();
void setLEDsColor(uint32_t color);

void setup() {
  Serial.begin(115200); // Inicializar la comunicación
  Serial.println("Setup iniciando");

  // Inicializar micro-ROS
  set_microros_transports();

  // Inicializar nodos, publicadores y mensajes
  rcl_allocator_t allocator = rcl_get_default_allocator();

  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "micro_ros_node", "", &support);

  rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "distance");

  pinMode(trigPin, OUTPUT); // Trig como output
  pinMode(echoPin, INPUT); // Echo como input
  // PWM motors
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Motores
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  analogWrite(ENA, LOW);
  analogWrite(ENB, LOW);

  // Inicializar la tira de LED
  strip.begin();
  strip.show(); // Inicializa todos los LEDs apagados

  Serial.println("Setup completado");
}

void loop() {
  Serial.println("Starting loop...");
  read_sensor();
  delay(100);

  // Publicar la distancia medida
  distance_msg.data = distance;
  rcl_ret_t ret = rcl_publish(&publisher, &distance_msg, NULL);
  if (ret != RCL_RET_OK) {
    Serial.print("Error publicando mensaje: ");
    Serial.println(ret);
  }

  // Mover hacia adelante si no hay obstáculo
  if (safetyDistance > 35) {
    Serial.println("Moviendo hacia adelante");
    digitalWrite(IN1, LOW);
    digitalWrite(IN4, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    analogWrite(ENA, 150);
    analogWrite(ENB, 150);
    setLEDsColor(strip.Color(0, 255, 0)); // Verde
  } else {
    // Detenerse y girar si hay obstáculo
    Serial.println("Obstáculo detectado, girando");
    digitalWrite(IN1, LOW);
    digitalWrite(IN4, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    analogWrite(ENA, LOW);
    analogWrite(ENB, LOW);
    delay(500);

    // Girar
    digitalWrite(IN1, HIGH);
    digitalWrite(IN4, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    analogWrite(ENA, 150);
    analogWrite(ENB, 150);
    setLEDsColor(strip.Color(255, 0, 0)); // Rojo
    delay(1000);
  }
}

void read_sensor() {
  // Limpia el triger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Activa el trig por 10 microsegundos
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Lee el echo
  duration = pulseIn(echoPin, HIGH);

  // Calcula la distancia
  distance = duration * 0.034 / 2;
  safetyDistance = distance;

  // Imprime la distancia en el monitor serial
  Serial.print("Distance: ");
  Serial.println(distance);
}

void setLEDsColor(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}
