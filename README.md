# Усунення брязкоту контактів кнопки (debounce)
Проект реалізований на Arduino https://github.com/voychenko-v/ba_embedded_esp32/tree/09_debounce перероблено на espidf 

## Polling + FSM (без interrupts)
- Oпитування кожні 10 мс
- Debounce як FSM: `IDLE → PRESS_DEBOUNCE → PRESSED → RELEASE_DEBOUNCE → IDLE`
