*** Settings ***
Library    ../keywords/climate_keywords.py
Library    OperatingSystem


*** Test Cases ***
Enable HVAC Power

    Set HVAC Power    ${True}

    Sleep    2s


Enable AC

    Set AC    ${True}

    Sleep    1s


Verify Fan Speed 5

    Set Fan Speed    5

    Sleep    1s

    ${speed}=    Get Fan Speed

    Should Be Equal As Integers    ${speed}    5


Set Temperature To 22

    Set Temperature    22

    Sleep    1s


Enable Dual Mode

    Set Dual Mode    ${True}
