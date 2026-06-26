*** Settings ***
Library    ../keywords/climate_keywords.py


*** Test Cases ***
Climate Full Regression

    Set HVAC Power    ${True}

    Set AC    ${True}

    Set Fan Speed    3

    Set Temperature    21

    Set Dual Mode    ${True}
