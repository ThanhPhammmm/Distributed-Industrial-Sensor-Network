#include "SlaveRegistry.h"

static const uint8_t     k_addrs[MAX_SLAVE_SLOTS] = SLAVE_SLOT_ADDRS;
static SlaveSlot_t       g_slots[MAX_SLAVE_SLOTS];
static SemaphoreHandle_t g_mtx;

void Registry_Init(void){
    g_mtx = xSemaphoreCreateMutex();
    configASSERT(g_mtx);
    for (uint8_t i = 0; i < MAX_SLAVE_SLOTS; i++) {
        memset(&g_slots[i], 0, sizeof(SlaveSlot_t));
        g_slots[i].addr  = k_addrs[i];
        g_slots[i].state = SREG_UNREGISTERED;
    }
}

void Registry_SetState(uint8_t idx, eSlaveRegState s){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].state = s;
    xSemaphoreGive(g_mtx);
}

SlaveSlot_t Registry_GetSlot(uint8_t idx){
    SlaveSlot_t snap;
    memset(&snap, 0, sizeof(snap));
    if (idx >= MAX_SLAVE_SLOTS) return snap;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    snap = g_slots[idx];
    xSemaphoreGive(g_mtx);
    return snap;
}

void Registry_SetConfigVersion(uint8_t idx, uint8_t ver){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].configVersion = ver;
    xSemaphoreGive(g_mtx);
}

void Registry_SetSensorTable(uint8_t idx, uint8_t count, const SensorDesc_t *d){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].sensorCount = (count > MAX_SENSORS_PER_SLAVE) ? MAX_SENSORS_PER_SLAVE : count;
    if (g_slots[idx].sensorCount && d)
    	memcpy(g_slots[idx].sensors, d, g_slots[idx].sensorCount * sizeof(SensorDesc_t));
    g_slots[idx].state = SREG_READY;
    xSemaphoreGive(g_mtx);
}

void Registry_UpdateReading(uint8_t idx, uint8_t sensorId, eDataType dt, SensorReading_t reading){
    if (idx >= MAX_SLAVE_SLOTS) return;
    if (sensorId < 1U || sensorId > MAX_SENSORS_PER_SLAVE) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].lastReading[sensorId - 1U] = reading;
    /* Keep cached dataType in sync */
    for (uint8_t i = 0; i < g_slots[idx].sensorCount; i++) {
        if (g_slots[idx].sensors[i].sensorId == sensorId) {
            g_slots[idx].sensors[i].dataType = (uint8_t)dt;
            break;
        }
    }
    xSemaphoreGive(g_mtx);
}

void Registry_SetLastSeen(uint8_t idx, uint32_t ms){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].lastSeenMs = ms;
    xSemaphoreGive(g_mtx);
}

void Registry_SetMissedPolls(uint8_t idx, uint8_t n){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].missedPolls = n;
    xSemaphoreGive(g_mtx);
}

void Registry_IncrementPoll(uint8_t idx){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].stats.totalPolls++;
    xSemaphoreGive(g_mtx);
}

void Registry_SetRegistered(uint8_t idx, bool value){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].registered = value;
    xSemaphoreGive(g_mtx);
}

void Registry_IncrementNack(uint8_t idx){
    if (idx >= MAX_SLAVE_SLOTS) return;
    xSemaphoreTake(g_mtx, portMAX_DELAY);
    g_slots[idx].stats.nacks++;
    xSemaphoreGive(g_mtx);
}

