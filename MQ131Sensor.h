#pragma once

#include <stdint.h>

#define MQ131_DEFAULT_R_LOAD 10000
#define MQ131_DEFAULT_R0_SENSOR 30000.0f

struct MQ131Sensor
{
    enum class Unit
    {
        PPM,
        PPB,
        MG_M3,
        UG_M3
    };

    struct Env
    {
        int temperature;
        int humidity;
    };

    MQ131Sensor( int pin_sensor, uint32_t r_load = MQ131_DEFAULT_R_LOAD );

    float get_o3( Unit unit, const Env& env ) const;

    inline float
    get_r0_sensor( ) const
    {
        return r0_sensor_;
    }

    inline void
    set_r0_sensor( float r0_sensor )
    {
        r0_sensor_ = r0_sensor;
    }

    void calibrate( );

private:
    float read_r_sensor( ) const;

    inline static const Env&
    get_default_env( )
    {
        return default_env;
    }

    static float get_env_correction_ratio( const Env& env );

private:
    const int pin_sensor_;
    const uint32_t r_load_;
    float r0_sensor_;

    static const Env default_env;
};