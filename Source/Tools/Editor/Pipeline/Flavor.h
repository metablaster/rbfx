//
// Copyright (c) 2017-2019 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include <EASTL/map.h>
#include <EASTL/string.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Variant.h>

namespace Urho3D
{

static const ea::string DEFAULT_PIPELINE_FLAVOR{"default"};

class Flavor : public Object
{
    URHO3D_OBJECT(Flavor, Object);
public:
    using EngineParametersMap = ea::map<ea::string, Variant>;

    /// Construct.
    explicit Flavor(Context* context);
    /// Return sname of this flavor.
    const ea::string& GetName() const { return name_; }
    /// Set name of this flavor.
    void SetName(const ea::string& newName);
    /// Returns map of engine parameters specific to this flavor.
    const EngineParametersMap& GetEngineParameters() const { return engineParameters_; }
    /// Returns map of engine parameters specific to this flavor.
    EngineParametersMap& GetEngineParameters() { return engineParameters_; }
    /// Returns true if this is a default flavor.
    bool IsDefault() const { return isDefault_; }
    /// Returns true if flavor is supposed to be imported during runtime of editor. TODO: Allow user to configure this value.
    bool IsImportedByDefault() const { return IsDefault(); }
    /// Returns hash of this flavor.
    StringHash ToHash() const { return StringHash(name_); }

    /// Equality operator.
    bool operator ==(const Flavor& rhs) const { return name_ == rhs.name_; }

protected:
    /// Flavor name.
    ea::string name_{};
    /// Engine parameters specific to this flavor. Player will fill Application::engineParameters_ with these values.
    EngineParametersMap engineParameters_;
    /// Flag indicating that this flavor is default.
    bool isDefault_ = false;
};

}