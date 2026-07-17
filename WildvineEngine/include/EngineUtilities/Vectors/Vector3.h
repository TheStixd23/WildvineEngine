/*
 * MIT License
 *
 * Copyright (c) 2024 Roberto Charreton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * In addition, any project or software that uses this library or class must include
 * the following acknowledgment in the credits:
 *
 * "This project uses software developed by Roberto Charreton and Attribute Overload."
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include "EngineUtilities\Utilities\EngineMath.h"

namespace EU {
    class Vector3 {
    public:
        float x;
        float y;
        float z;

        Vector3()
            : x(0.0f), y(0.0f), z(0.0f) {
        }

        Vector3(float xValue, float yValue, float zValue)
            : x(xValue), y(yValue), z(zValue) {
        }

        Vector3 operator+(const Vector3& other) const {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        Vector3& operator+=(const Vector3& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3 operator-(const Vector3& other) const {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        Vector3& operator-=(const Vector3& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vector3 operator-() const {
            return Vector3(-x, -y, -z);
        }

        Vector3 operator*(float scalar) const {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }

        Vector3& operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vector3 operator/(float scalar) const {
            if (EU::abs(scalar) <= 0.000001f) {
                return Vector3();
            }
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        Vector3& operator/=(float scalar) {
            if (EU::abs(scalar) > 0.000001f) {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }
            return *this;
        }

        float magnitudeSquared() const {
            return x * x + y * y + z * z;
        }

        float magnitude() const {
            return EU::sqrt(magnitudeSquared());
        }

        bool isNearlyZero(float tolerance = 0.000001f) const {
            return magnitudeSquared() <= tolerance * tolerance;
        }

        Vector3 normalize() const {
            const float length = magnitude();
            if (length <= 0.000001f) {
                return Vector3();
            }
            return *this / length;
        }

        void zero() {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }

        void one() {
            x = 1.0f;
            y = 1.0f;
            z = 1.0f;
        }

        static float dot(const Vector3& a, const Vector3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static Vector3 cross(const Vector3& a, const Vector3& b) {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
        }

        float* data() {
            return &x;
        }

        const float* data() const {
            return &x;
        }
    };
}
