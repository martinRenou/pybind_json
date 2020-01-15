/***************************************************************************
* Copyright (c) 2019, Martin Renou                                         *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef PYBIND11_JSON_HPP
#define PYBIND11_JSON_HPP

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "pybind11/pybind11.h"

namespace py = pybind11;

namespace nlohmann
{

    namespace detail
    {
        inline py::object from_json_impl(const json& j)
        {
            if (j.is_null())
            {
                return py::none();
            }
            else if (j.is_boolean())
            {
                return py::bool_(j.get<bool>());
            }
            else if (j.is_number())
            {
                double number = j.get<double>();
                if (number == std::floor(number))
                {
                    return py::int_(j.get<long>());
                }
                else
                {
                    return py::float_(number);
                }
            }
            else if (j.is_string())
            {
                return py::str(j.get<std::string>());
            }
            else if (j.is_array())
            {
                py::list obj;
                for (const auto& el : j)
                {
                    obj.attr("append")(from_json_impl(el));
                }
                return obj;
            }
            else // Object
            {
                py::dict obj;
                for (json::const_iterator it = j.cbegin(); it != j.cend(); ++it)
                {
                    obj[py::str(it.key())] = from_json_impl(it.value());
                }
                return obj;
            }
        }

        inline json to_json_impl(const py::handle& obj)
        {
            if (obj.is_none())
            {
                return nullptr;
            }
            if (py::isinstance<py::bool_>(obj))
            {
                return obj.cast<bool>();
            }
            if (py::isinstance<py::int_>(obj))
            {
                return obj.cast<long>();
            }
            if (py::isinstance<py::float_>(obj))
            {
                return obj.cast<double>();
            }
            if (py::isinstance<py::str>(obj))
            {
                return obj.cast<std::string>();
            }
            if (py::isinstance<py::tuple>(obj) || py::isinstance<py::list>(obj))
            {
                auto out = json::array();
                for (const py::handle& value : obj)
                {
                    out.push_back(to_json_impl(value));
                }
                return out;
            }
            if (py::isinstance<py::dict>(obj))
            {
                auto out = json::object();
                for (const py::handle& key : obj)
                {
                    out[py::str(key).cast<std::string>()] = to_json_impl(obj[key]);
                }
                return out;
            }
            throw std::runtime_error("to_json not implemented for this type of object: " + obj.cast<std::string>());
        }
    }

    template <>
    struct adl_serializer<py::object>
    {
        inline static py::object from_json(const json& j)
        {
            return detail::from_json_impl(j);
        }

        inline static void to_json(json& j, const py::object& obj)
        {
            j = detail::to_json_impl(obj);
        }
    };

}

namespace pybind11
{
    namespace detail
    {
        template <> struct type_caster<nlohmann::json>
        {
        public:
            /**
             * This macro establishes the name 'user_type' in
             * function signatures and declares a local variable 'value' of user type
             */
            PYBIND11_TYPE_CASTER(nlohmann::json, _("json"));

            /**
             * Conversion part 1 (Python->C++): convert a PyObject into a inty
             * instance or return false upon failure. The second argument
             * indicates whether implicit conversions should be applied.
             */
            bool load(handle src, bool)
            {
                /* Extract PyObject from handle */
                /* Now try to convert into a C++ type */
                value = nlohmann::detail::to_json_impl(src);
                if (value.is_null())
                    return false;
                return true;
            }

            /**
             * Conversion part 2 (C++ -> Python): convert an inty instance into
             * a Python object. The second and third arguments are used to
             * indicate the return value policy and parent object (for
             * ``return_value_policy::reference_internal``) and are generally
             * ignored by implicit casters.
             */
            static handle cast(nlohmann::json src, return_value_policy /* policy */, handle /* parent */)
            {
                object pyo = nlohmann::detail::from_json_impl(src);
                return pyo.release();
            }
        };
    } // namespace detail
} // namespace pybind11

#endif
