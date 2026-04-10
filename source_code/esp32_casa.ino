// ========== CONFIGURACIÓN DE PINES ==========
#define LED_PIN 5      // Cambia al pin que uses (13 es recomendado para evitar conflicto)
#define TRIG_PIN 2      // D2 - Pin de trigger del sensor ultrasónico
#define ECHO_PIN 4      // D4 - Pin de echo del sensor ultrasónico

#include <ESP32Servo.h>  // Librería específica para ESP32

Servo miServo;
int pinServo = 15;

// ========== VARIABLES PARA LED ==========
bool blinkActive = false;
unsigned long previousMillis = 0;
int blinkOnTime = 500;    // Tiempo encendido en ms
int blinkOffTime = 500;   // Tiempo apagado en ms
bool ledState = LOW;
bool manualMode = true;    // true = control manual ON/OFF, false = modo parpadeo

// ========== VARIABLES PARA SENSOR ULTRASÓNICO ==========
unsigned long lastDistanceRead = 0;
const unsigned long distanceReadInterval = 200;  // Leer distancia cada 200ms
float lastDistance = -1;  // Última distancia medida en cm
bool distanceValid = false;

// ========== VARIABLES PARA ACCIONES AUTOMÁTICAS ==========
bool autoMode = false;           // Modo automático basado en distancia
float distanceThreshold = 30.0;  // Umbral en cm para acciones automáticas
bool autoActionEnabled = false;   // Si se ha habilitado acción automática

void setup() {
  Serial.begin(115200);
  
  // Configurar LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configurar sensor ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Asigna el timer y el canal PWM automáticamente
  miServo.attach(pinServo, 500, 2500);  // Min 500µs, Max 2500µs
  
  
  Serial.println("=== ESP32 Listo ===");
  Serial.println("Sistema con LED y Sensor Ultrasónico");
  Serial.println("Escribe HELP para ver comandos disponibles");
  Serial.println();
  
  // Medición inicial
  delay(100);
  medirDistancia();
}

void loop() {
  // 1. Procesar comandos serial
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    procesarComando(comando);
  }
  
  // 2. Medir distancia periódicamente
  if (millis() - lastDistanceRead >= distanceReadInterval) {
    lastDistanceRead = millis();
    medirDistancia();
    
    // 3. Acciones automáticas basadas en distancia
    if (autoActionEnabled && distanceValid) {
      manejarAccionAutomatica();
    }
  }
  
  // 4. Manejar parpadeo del LED (si está activo)
  if (blinkActive && !manualMode && !autoMode) {
    unsigned long currentMillis = millis();
    unsigned long interval = ledState ? blinkOnTime : blinkOffTime;
    
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}

// ========== FUNCIONES DEL SENSOR ULTRASÓNICO ==========
void medirDistancia() {
  // Asegurar que el pin TRIG esté en LOW
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Enviar pulso de 10 microsegundos
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Medir duración del pulso en ECHO
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // Timeout 30ms (máx ~5m)
  
  if (duration == 0) {
    // Timeout: no se recibió eco
    distanceValid = false;
    lastDistance = -1;
  } else {
    // Calcular distancia: duración * velocidad del sonido (343 m/s) / 2
    // distancia (cm) = duration (us) * 0.0343 / 2
    lastDistance = duration * 0.0343 / 2;
    distanceValid = true;
    
    // Limitar valores fuera de rango
    if (lastDistance > 400) {
      lastDistance = 400;
    }
  }
}

float obtenerDistancia() {
  if (!distanceValid) {
    return -1;
  }
  return lastDistance;
}

// ========== FUNCIONES DEL LED ==========
void encenderLED() {
  // Desactivar modos automáticos
  autoMode = false;
  blinkActive = false;
  manualMode = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED encendido manualmente");
}

void apagarLED() {
  // Desactivar modos automáticos
  autoMode = false;
  blinkActive = false;
  manualMode = true;
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED apagado manualmente");
}

void iniciarParpadeo(int onTime, int offTime) {
  blinkOnTime = onTime;
  blinkOffTime = offTime;
  blinkActive = true;
  manualMode = false;
  autoMode = false;
  ledState = LOW;
  digitalWrite(LED_PIN, LOW);
  previousMillis = millis();
  Serial.print("Modo parpadeo activado: ON=");
  Serial.print(blinkOnTime);
  Serial.print("ms, OFF=");
  Serial.print(blinkOffTime);
  Serial.println("ms");
}

void detenerParpadeo() {
  blinkActive = false;
  if (!autoMode) {
    manualMode = true;
    digitalWrite(LED_PIN, LOW);
  }
  Serial.println("Parpadeo detenido");
}

// ========== FUNCIONES DE ACCIONES AUTOMÁTICAS ==========
void habilitarAutoMode(bool habilitar) {
  autoActionEnabled = habilitar;
  if (habilitar) {
    Serial.print("Modo automático ACTIVADO. LED se encenderá cuando distancia < ");
    Serial.print(distanceThreshold);
    Serial.println(" cm");
    
    // Realizar una acción inmediata basada en la distancia actual
    if (distanceValid) {
      manejarAccionAutomatica();
    }
  } else {
    autoMode = false;
    Serial.println("Modo automático DESACTIVADO");
    // Restaurar control manual
    manualMode = true;
    digitalWrite(LED_PIN, LOW);
  }
}

void manejarAccionAutomatica() {
  if (lastDistance < distanceThreshold) {
    // Objeto detectado dentro del umbral
    if (!autoMode) {
      autoMode = true;
      manualMode = false;
      blinkActive = false;
      digitalWrite(LED_PIN, HIGH);
      Serial.print("¡Objeto detectado! Distancia: ");
      Serial.print(lastDistance);
      Serial.println(" cm - LED encendido");
    }
  } else {
    // Objeto fuera del umbral
    if (autoMode) {
      autoMode = false;
      manualMode = true;
      blinkActive = false;
      digitalWrite(LED_PIN, LOW);
      Serial.print("Objeto fuera de rango. Distancia: ");
      Serial.print(lastDistance);
      Serial.println(" cm - LED apagado");
    }
  }
}

void setDistanceThreshold(float umbral) {
  if (umbral > 0 && umbral < 400) {
    distanceThreshold = umbral;
    Serial.print("Umbral de distancia cambiado a: ");
    Serial.print(distanceThreshold);
    Serial.println(" cm");
  } else {
    Serial.println("Umbral inválido. Usa un valor entre 1 y 400 cm");
  }
}


void abrirPuerta(){
  Serial.println("Moviendo a 90°");
  miServo.write(90);
  delay(1000);
}


void cerrarPuerta(){
  Serial.println("Moviendo a 0°");
  miServo.write(0);
  delay(1000);
}


// ========== COMANDOS SERIAL ==========
void procesarComando(String comando) {
  comando.toUpperCase();
  
  if (comando == "ON") {
    encenderLED();
  }
  else if (comando == "OFF") {
    apagarLED();
  }
  else if (comando == "STATUS") {
    mostrarEstado();
  }
  else if (comando.startsWith("BLINK")) {
    procesarComandoBlink(comando);
  }
  else if (comando == "DISTANCIA" || comando == "DIST") {
    mostrarDistancia();
  }
  else if (comando.startsWith("AUTO")) {
    procesarComandoAuto(comando);
  }
  else if (comando.startsWith("UMBRAL")) {
    procesarComandoUmbral(comando);
  }
  else if (comando == "HELP") {
    mostrarAyuda();
  }
  else if (comando == "CERRAR"){
    cerrarPuerta();
  }
  else if (comando == "ABRIR"){
    abrirPuerta();
  }
  else {
    Serial.println("Comando no reconocido. Escribe HELP para ayuda.");
  }
}

void procesarComandoBlink(String comando) {
  String params = comando.substring(6);
  params.trim();
  
  if (params == "ON") {
    iniciarParpadeo(blinkOnTime, blinkOffTime);
  }
  else if (params == "OFF") {
    detenerParpadeo();
  }
  else if (params.startsWith("SPEED")) {
    int speed = params.substring(6).toInt();
    if (speed > 0 && speed <= 5000) {
      blinkOnTime = speed;
      blinkOffTime = speed;
      Serial.print("Velocidad cambiada a: ");
      Serial.print(speed);
      Serial.println(" ms");
      
      if (blinkActive && !manualMode && !autoMode) {
        previousMillis = millis();
      }
    } else {
      Serial.println("Velocidad inválida. Usa 1-5000 ms");
    }
  }
  else if (params.startsWith("CUSTOM")) {
    params = params.substring(7);
    params.trim();
    
    int espacio = params.indexOf(' ');
    if (espacio > 0) {
      int onTime = params.substring(0, espacio).toInt();
      int offTime = params.substring(espacio + 1).toInt();
      
      if (onTime > 0 && onTime <= 10000 && offTime > 0 && offTime <= 10000) {
        blinkOnTime = onTime;
        blinkOffTime = offTime;
        Serial.print("Parpadeo personalizado: ON=");
        Serial.print(blinkOnTime);
        Serial.print("ms, OFF=");
        Serial.print(blinkOffTime);
        Serial.println("ms");
        
        if (blinkActive && !manualMode && !autoMode) {
          previousMillis = millis();
        }
      } else {
        Serial.println("Tiempos inválidos. Usa 1-10000 ms");
      }
    } else {
      Serial.println("Formato: BLINK CUSTOM [on_ms] [off_ms]");
    }
  }
  else {
    Serial.println("Subcomandos BLINK: ON, OFF, SPEED, CUSTOM");
  }
}

void procesarComandoAuto(String comando) {
  String params = comando.substring(5);
  params.trim();
  
  if (params == "ON") {
    habilitarAutoMode(true);
  }
  else if (params == "OFF") {
    habilitarAutoMode(false);
  }
  else {
    Serial.println("Usa: AUTO ON o AUTO OFF");
  }
}

void procesarComandoUmbral(String comando) {
  String params = comando.substring(7);
  params.trim();
  
  float umbral = params.toFloat();
  if (umbral > 0 && umbral < 400) {
    setDistanceThreshold(umbral);
  } else {
    Serial.println("Formato: UMBRAL [cm] (1-400)");
  }
}

void mostrarEstado() {
  Serial.println("=== ESTADO DEL SISTEMA ===");
  
  // Estado del LED
  if (autoMode) {
    Serial.println("Modo LED: AUTOMÁTICO (por distancia)");
    Serial.print("  LED actualmente: ");
    Serial.println(digitalRead(LED_PIN) ? "ENCENDIDO" : "APAGADO");
  } else if (blinkActive && !manualMode) {
    Serial.println("Modo LED: PARPADEO");
    Serial.print("  ON: ");
    Serial.print(blinkOnTime);
    Serial.print("ms, OFF: ");
    Serial.print(blinkOffTime);
    Serial.println("ms");
  } else if (manualMode) {
    Serial.print("Modo LED: MANUAL - ");
    Serial.println(digitalRead(LED_PIN) ? "ENCENDIDO" : "APAGADO");
  }
  
  // Estado del sensor
  Serial.println("\n=== SENSOR ULTRASÓNICO ===");
  if (distanceValid) {
    Serial.print("Distancia: ");
    Serial.print(lastDistance);
    Serial.println(" cm");
  } else {
    Serial.println("Distancia: No disponible (objeto fuera de rango o error)");
  }
  
  // Configuración automática
  Serial.println("\n=== CONFIGURACIÓN AUTO ===");
  Serial.print("Modo automático: ");
  Serial.println(autoActionEnabled ? "ACTIVADO" : "DESACTIVADO");
  if (autoActionEnabled) {
    Serial.print("Umbral: ");
    Serial.print(distanceThreshold);
    Serial.println(" cm");
  }
  
  Serial.println("=========================");
}

void mostrarDistancia() {
  if (distanceValid) {
    Serial.print("Distancia medida: ");
    Serial.print(lastDistance);
    Serial.println(" cm");
  } else {
    Serial.println("Error: No se pudo medir distancia");
    Serial.println("Verifica conexiones del sensor ultrasónico");
  }
}

void mostrarAyuda() {
  Serial.println("\n=== COMANDOS DISPONIBLES ===");
  Serial.println("\n--- Control LED ---");
  Serial.println("ON                    - Enciende LED");
  Serial.println("OFF                   - Apaga LED");
  Serial.println("BLINK ON              - Inicia parpadeo");
  Serial.println("BLINK OFF             - Detiene parpadeo");
  Serial.println("BLINK SPEED [ms]      - Velocidad de parpadeo");
  Serial.println("BLINK CUSTOM [on] [off] - Parpadeo personalizado");
  
  Serial.println("\n--- Sensor Ultrasónico ---");
  Serial.println("DISTANCIA o DIST      - Muestra distancia actual");
  Serial.println("AUTO ON               - Activa modo automático");
  Serial.println("AUTO OFF              - Desactiva modo automático");
  Serial.println("UMBRAL [cm]           - Configura umbral (1-400)");
  
  Serial.println("\n--- Información ---");
  Serial.println("STATUS                - Muestra estado completo");
  Serial.println("HELP                  - Muestra esta ayuda");
  Serial.println("============================\n");
}
