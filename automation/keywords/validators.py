def validate_fan_speed(value):
    if value < 0 or value > 7:
        raise ValueError(
            f"Invalid fan speed: {value}"
        )


def validate_temperature(value):
    if value < 16 or value > 32:
        raise ValueError(
            f"Invalid temperature: {value}"
        )


def validate_boolean(value):
    if type(value) is not bool:
        raise ValueError(
            "Value must be True or False"
        )
