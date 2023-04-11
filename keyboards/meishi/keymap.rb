
pin = 42
gpio_init pin
gpio_set_dir pin, 1
gpio_put pin, 0

while true
    sleep_ms 500
    gpio_put pin, 1
    sleep_ms 500
    gpio_put pin, 0
end

