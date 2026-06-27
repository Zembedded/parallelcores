#include <Arduino.h>

// --- CPU-intensive Arbeit: Primzahlen zählen ---
static int countPrimes(int limit) {
  int count = 0;
  for (int n = 2; n < limit; n++) {
    bool isPrime = true;
    for (int d = 2; d * d <= n; d++) {
      if (n % d == 0) { isPrime = false; break; }
    }
    if (isPrime) count++;
  }
  return count;
}

// --- Synchronisation ---
static volatile bool taskADone = false;
static volatile bool taskBDone = false;
static volatile int  taskAResult = 0;
static volatile int  taskBResult = 0;

static const int WORK_LIMIT = 50000;

void heavyTaskA(void *param) {
  Serial.printf("    [Core %d] Task A startet...\n", xPortGetCoreID());
  taskAResult = countPrimes(WORK_LIMIT);
  Serial.printf("    [Core %d] Task A fertig – %d Primzahlen\n", xPortGetCoreID(), taskAResult);
  taskADone = true;
  vTaskDelete(NULL);
}

void heavyTaskB(void *param) {
  Serial.printf("    [Core %d] Task B startet...\n", xPortGetCoreID());
  taskBResult = countPrimes(WORK_LIMIT);
  Serial.printf("    [Core %d] Task B fertig – %d Primzahlen\n", xPortGetCoreID(), taskBResult);
  taskBDone = true;
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println("========================================");
  Serial.println("  ESP32-S3 Dual-Core Parallelitäts-Beweis");
  Serial.println("========================================\n");

  // ---- TEST 1: Beide Tasks auf EINEN Core gepinnt ----
  Serial.println(">>> TEST 1: Beide Tasks auf Core 1 gepinnt (sequentiell)");
  taskADone = false;
  taskBDone = false;

  unsigned long seqStart = millis();

  xTaskCreatePinnedToCore(heavyTaskA, "TaskA", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(heavyTaskB, "TaskB", 4096, NULL, 1, NULL, 1);

  while (!taskADone || !taskBDone) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  unsigned long seqTime = millis() - seqStart;
  Serial.printf("    Dauer: %lu ms\n\n", seqTime);

  // ---- TEST 2: Tasks auf verschiedene Cores verteilt ----
  Serial.println(">>> TEST 2: Task A auf Core 0, Task B auf Core 1 (parallel)");
  taskADone = false;
  taskBDone = false;

  unsigned long parStart = millis();

  xTaskCreatePinnedToCore(heavyTaskA, "TaskA", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(heavyTaskB, "TaskB", 4096, NULL, 1, NULL, 1);

  while (!taskADone || !taskBDone) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  unsigned long parTime = millis() - parStart;
  Serial.printf("    Dauer: %lu ms\n\n", parTime);

  // ---- Auswertung ----
  Serial.println("========================================");
  Serial.printf("  Ein Core:    %lu ms\n", seqTime);
  Serial.printf("  Zwei Cores:  %lu ms\n", parTime);
  Serial.printf("  Speedup:     %.2fx\n", (float)seqTime / (float)parTime);
  Serial.println("========================================");

  if (parTime < seqTime * 0.75) {
    Serial.println("  ==> BEWEIS: Tasks liefen ECHT PARALLEL!");
  } else {
    Serial.println("  ==> Kein signifikanter Speedup (unerwartet).");
  }
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(10000));
}