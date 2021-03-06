//
//  LightBall.js
//  examples/lightBall
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creats a particle light ball which makes particle trails as you move it.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");

LightBall = function(spawnPosition) {

    var colorPalette = [{
        red: 25,
        green: 20,
        blue: 162
    }];


    var containerBall = Entities.addEntity({
        type: "Sphere",
        name: "containerBall",
        position: Vec3.sum(spawnPosition, {
            x: 0,
            y: 0.5,
            z: 0
        }),
        dimensions: {
            x: 0.1,
            y: 0.1,
            z: 0.1
        },
        color: {
            red: 15,
            green: 10,
            blue: 150
        },
        dynamic: true,
        // gravity: {
        //     x: 0,
        //     y: -0.5,
        //     z: 0
        // },
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    relativePosition: {
                        x: 0,
                        y: 0.1,
                        z: 0
                    }
                },
                invertSolidWhileHeld: true
            }
        })
    });


    var light = Entities.addEntity({
        type: 'Light',
        name: "ballLight",
        parentID: containerBall,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: colorPalette[randInt(0, colorPalette.length)],
        intensity: 5
    });


    var lightBall = Entities.addEntity({
        type: "ParticleEffect",
        parentID: containerBall,
        isEmitting: true,
        name: "particleBall",
        colorStart: {
            red: 200,
            green: 20,
            blue: 40
        },
        color: {
            red: 200,
            green: 200,
            blue: 255
        },
        colorFinish: {
            red: 25,
            green: 20,
            blue: 255
        },
        maxParticles: 100000,
        lifespan: 2,
        emitRate: 10000,
        emitSpeed: 0.1,
        lifetime: -1,
        speedSpread: 0.0,
        emitDimensions: {
            x: 0,
            y: 0,
            z: 0
        },
        polarStart: 0,
        polarFinish: Math.PI,
        azimuthStart: -Math.PI,
        azimuthFinish: Math.PI,
        emitAcceleration: {
            x: 0,
            y: 0,
            z: 0
        },
        accelerationSpread: {
            x: 0.00,
            y: 0.00,
            z: 0.00
        },
        particleRadius: 0.02,
        radiusSpread: 0,
        radiusStart: 0.03,
        radiusFinish: 0.0003,
        alpha: 0,
        alphaSpread: 0.5,
        alphaStart: 0,
        alphaFinish: 0.5,
        textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        emitterShouldTrail: true
    })



    function cleanup() {
        Entities.deleteEntity(lightBall);
        Entities.deleteEntity(containerBall);
        Entities.deleteEntity(light);
    }

    this.cleanup = cleanup;
}
