#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- CONFIGURAÇÕES ---
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define REDE_FREQUENCIA 60
#define OFFSET_ADC 512
#define LIMIAR_QUEDA_V 10.0f

float calibration_factor = 0.85;

// -----------------------------------------------------------------------------
// Contador de tempo relativo (millis) via Timer0
// O ATmega328P não tem RTC. A data/hora absoluta é atribuída pelo ESP01/ESP32
// (que possui NTP) a partir deste timestamp relativo.
// -----------------------------------------------------------------------------
volatile unsigned long millis_count = 0;

ISR(TIMER0_COMPA_vect) {
    millis_count++;
}

void timer0_init_ms() {
    // CTC mode, prescaler 64: F_CPU=16MHz -> 250kHz -> OCR0A=249 => 1ms
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A  = 249;
    TIMSK0 = (1 << OCIE0A);
}

unsigned long millis_get() {
    unsigned long v;
    uint8_t sreg = SREG;
    cli();
    v = millis_count;
    SREG = sreg;
    return v;
}

// Inicialização da comunicação Serial (UART)
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0); // Ativa transmissão
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8 bits de dados
}

// Enviar um caractere via Serial
void uart_putchar(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// Enviar uma string via Serial
void uart_puts(const char* s) {
    while (*s) uart_putchar(*s++);
}

// Inicialização do ADC (Conversor Analógico-Digital)
void adc_init() {
    ADMUX = (1 << REFS0); // Referência AVcc (5V) e pino A0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}

// Ler valor analógico
uint16_t adc_read() {
    ADCSRA |= (1 << ADSC); // Inicia conversão
    while (ADCSRA & (1 << ADSC)); // Espera terminar
    return ADC;
}

float get_voltage_rms() {
    unsigned long samples = 0;
    long long sum_sq = 0;

    // Aproximadamente 2 ciclos de 60Hz
    for (int i = 0; i < 3000; i++) {
        int raw = adc_read();
        int centered = raw - OFFSET_ADC;
        sum_sq += (long)centered * centered;
        samples++;
        _delay_us(10);
    }

    float mean_sq = (float)sum_sq / samples;
    return sqrt(mean_sq);
}

int main(void) {
    uart_init(MYUBRR);
    adc_init();
    timer0_init_ms();
    sei();

    char buffer[32];
    char linha[96];

    // Estado da queda
    uint8_t em_queda = 0;
    unsigned long t_inicio_queda = 0;

    uart_puts("Monitor ZMPT101B - C Puro Iniciado\r\n");
    uart_puts("Timestamp relativo em ms (origem = boot do sensor).\r\n");
    uart_puts("Data/hora absoluta deve ser atribuida pelo ESP01/ESP32 via NTP.\r\n");

    while (1) {
        float valor_rms = get_voltage_rms();
        float voltagem_final = valor_rms * calibration_factor;
        unsigned long t = millis_get();

        if (voltagem_final < LIMIAR_QUEDA_V) {
            if (!em_queda) {
                em_queda = 1;
                t_inicio_queda = t;
                snprintf(linha, sizeof(linha),
                         "EVT QUEDA_INICIO t_ms=%lu\r\n", t_inicio_queda);
                uart_puts(linha);
            } else {
                // Continua em queda: publica heartbeat a cada ciclo
                snprintf(linha, sizeof(linha),
                         "EVT QUEDA_EM_CURSO t_ms=%lu dur_ms=%lu\r\n",
                         t, t - t_inicio_queda);
                uart_puts(linha);
            }
        } else {
            if (em_queda) {
                unsigned long dur = t - t_inicio_queda;
                em_queda = 0;
                snprintf(linha, sizeof(linha),
                         "EVT QUEDA_FIM t_ms=%lu dur_ms=%lu\r\n", t, dur);
                uart_puts(linha);
            }
            dtostrf(voltagem_final, 4, 2, buffer);
            snprintf(linha, sizeof(linha),
                     "MED t_ms=%lu vrms=%s V\r\n", t, buffer);
            uart_puts(linha);
        }

        _delay_ms(500);
    }
}
