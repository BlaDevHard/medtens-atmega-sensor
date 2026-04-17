#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- CONFIGURAÇÕES ---
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define REDE_FREQUENCIA 60
#define OFFSET_ADC 512

float calibration_factor = 0.85;

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
    // Calculamos o número de amostras baseado no tempo de 2 ciclos (33.3ms)
    // Em C puro, usamos um contador fixo para simplificar sem timers complexos
    unsigned long samples = 0;
    long long sum_sq = 0;
    
    // Aproximadamente 2 ciclos de 60Hz
    for (int i = 0; i < 3000; i++) {
        int raw = adc_read();
        int centered = raw - OFFSET_ADC;
        sum_sq += (long)centered * centered;
        samples++;
        _delay_us(10); // Pequeno intervalo entre amostras
    }
    
    float mean_sq = (float)sum_sq / samples;
    return sqrt(mean_sq);
}

int main(void) {
    uart_init(MYUBRR);
    adc_init();
    
    char buffer[50];
    uart_puts("Monitor ZMPT101B - C Puro Iniciado\r\n");

    while (1) {
        float valor_rms = get_voltage_rms();
        float voltagem_final = valor_rms * calibration_factor;

        if (voltagem_final < 10.0) {
            uart_puts("QUEDA NA REDE\r\n");
        } else {
            // Converte float para string para enviar via UART
            dtostrf(voltagem_final, 4, 2, buffer);
            uart_puts("Tensao AC: ");
            uart_puts(buffer);
            uart_puts(" V\r\n");
        }

        _delay_ms(500);
    }
}