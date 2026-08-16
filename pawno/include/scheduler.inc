// ============================================================
//  Santa Aurora Roleplay ? Scheduler Central
//  Modulo : core/scheduler.inc
//  Objetivo: um unico timer global gerenciando todos os ciclos
//            evita centenas de timers independentes
// ============================================================

#if defined _SA_SCHEDULER_INC
    #endinput
#endif
#define _SA_SCHEDULER_INC

// include removido

// ------------------------------------------------------------
// Estado interno
// ------------------------------------------------------------
static g_SchedulerTimer    = -1;
static g_TickCount         = 0;

// Intervalos de dispatch (em ticks de 1 segundo)
#define SCHED_1S    1
#define SCHED_5S    5
#define SCHED_10S   10
#define SCHED_30S   30
#define SCHED_60S   60
#define SCHED_5MIN  300

// ------------------------------------------------------------
// Eventos disparados pelo scheduler
// Modulos implementam esses forwards para receber ticks
// ------------------------------------------------------------
forward OnSchedulerTick(interval);      // chamado a cada intervalo ativo

// Ticks especializados para modulos que precisam
forward OnScheduler1Second();
forward OnScheduler5Seconds();
forward OnScheduler10Seconds();
forward OnScheduler30Seconds();
forward OnScheduler1Minute();
forward OnScheduler5Minutes();

// ------------------------------------------------------------
// Init / Shutdown
// ------------------------------------------------------------
stock Scheduler_Init() {
    g_TickCount = 0;
    g_SchedulerTimer = SetTimer("Scheduler_Tick", SCHEDULER_TICK_MS, true);
    printf("[SCHEDULER] Iniciado (timer ID: %d)", g_SchedulerTimer);
}

stock Scheduler_Shutdown() {
    if(g_SchedulerTimer != -1) {
        KillTimer(g_SchedulerTimer);
        g_SchedulerTimer = -1;
    }
    printf("[SCHEDULER] Encerrado.");
}

// ------------------------------------------------------------
// Tick principal ? chamado a cada 1 segundo
// ------------------------------------------------------------
forward Scheduler_Tick();
public Scheduler_Tick() {
    g_TickCount++;

    // Sempre: tick de 1 segundo
    CallLocalFunction("OnScheduler1Second", "");

    // A cada 5 segundos
    if(g_TickCount % SCHED_5S == 0)
        CallLocalFunction("OnScheduler5Seconds", "");

    // A cada 10 segundos
    if(g_TickCount % SCHED_10S == 0)
        CallLocalFunction("OnScheduler10Seconds", "");

    // A cada 30 segundos
    if(g_TickCount % SCHED_30S == 0)
        CallLocalFunction("OnScheduler30Seconds", "");

    // A cada 60 segundos
    if(g_TickCount % SCHED_60S == 0)
        CallLocalFunction("OnScheduler1Minute", "");

    // A cada 5 minutos
    if(g_TickCount % SCHED_5MIN == 0)
        CallLocalFunction("OnScheduler5Minutes", "");

    // Resetar contador a cada hora para evitar overflow
    if(g_TickCount >= 3600) g_TickCount = 0;
}

// ------------------------------------------------------------
// Utilitarios
// ------------------------------------------------------------
stock Scheduler_GetTick() {
    return g_TickCount;
}

