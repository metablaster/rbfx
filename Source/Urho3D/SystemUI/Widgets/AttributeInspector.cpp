//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../../SystemUI/SystemUI.h"
#include "../../Core/StringUtils.h"
#include "../../Scene/Serializable.h"
#include "../../Resource/ResourceCache.h"
#include "../../IO/FileSystem.h"
#include "AttributeInspector.h"

#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <tinyfiledialogs/tinyfiledialogs.h>


namespace Urho3D
{

AttributeInspector::AttributeInspector(Urho3D::Context* context)
    : Object(context)
{

}

void AttributeInspector::RenderAttributes(Serializable* item)
{
    /// If serializable changes clear value buffers so values from previous item do not appear when inspecting new item.
    if (lastSerializable_.Get() != item)
    {
        buffers_.Clear();
        lastSerializable_ = item;
    }

    ui::Columns(2);

    ui::TextUnformatted("Filter");
    ui::NextColumn();
    if (ui::Button(ICON_FA_UNDO))
        filter_.front() = 0;
    if (ui::IsItemHovered())
        ui::SetTooltip("Reset filter.");
    ui::SameLine();
    ui::PushID("FilterEdit");
    ui::InputText("", &filter_.front(), filter_.size() - 1);
    ui::PopID();
    ui::NextColumn();

    ui::PushID(item);
    const auto& attributes = *item->GetAttributes();
    for (const AttributeInfo& info: attributes)
    {
        if (info.mode_ & AM_NOEDIT)
            continue;

        if (filter_.front() && !info.name_.Contains(&filter_.front(), false))
            continue;

        ui::TextUnformatted(info.name_.CString());
        ui::NextColumn();

        Variant value = item->GetAttribute(info.name_);

        ui::PushID(info.name_.CString());

        if (ui::Button(ICON_FA_CARET_DOWN))
            ui::OpenPopup("Attribute Menu");

        if (ui::BeginPopup("Attribute Menu"))
        {
            if (ui::MenuItem("Reset to default"))
            {
                item->SetAttribute(info.name_, info.defaultValue_);
                item->ApplyAttributes();
            }

            // Allow customization of attribute menu.
            using namespace AttributeInspectorMenu;
            SendEvent(E_ATTRIBUTEINSPECTORMENU, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, &info);

            ImGui::EndPopup();
        }
        ui::SameLine();

        if (RenderSingleAttribute(info, value))
        {
            item->SetAttribute(info.name_, value);
            item->ApplyAttributes();
        }

        ui::PopID();
        ui::NextColumn();
    }
    ui::PopID();
    ui::Columns(1);
}

std::array<char, 0x1000>& AttributeInspector::GetBuffer(const String& name, const String& default_value)
{
    auto it = buffers_.Find(name);
    if (it == buffers_.End())
    {
        auto& buffer = buffers_[name];
        strncpy(&buffer[0], default_value.CString(), buffer.size() - 1);
        return buffer;
    }
    else
        return it->second_;
}

void AttributeInspector::RemoveBuffer(const String& name)
{
    buffers_.Erase(name);
}

bool AttributeInspector::RenderSingleAttribute(const AttributeInfo& info, Variant& value)
{
    const int int_min = M_MIN_INT;
    const int int_max = M_MAX_INT;
    const int int_step = 1;
    const float float_min = -14000.f;
    const float float_max = 14000.f;
    const float float_step = 0.01f;

    bool modified = false;
    const char** combo_values = nullptr;
    auto combo_values_num = 0;
    if (info.enumNames_)
    {
        combo_values = info.enumNames_;
        for (; combo_values[++combo_values_num];);
    }

    if (combo_values)
    {
        int current = value.GetInt();
        modified |= ui::Combo("", &current, combo_values, combo_values_num);
        if (modified)
            value = current;
    }
    else
    {
        switch (info.type_)
        {
        case VAR_NONE:
            ui::TextUnformatted("None");
            break;
        case VAR_INT:
        {
            // TODO: replace this with custom control that properly handles int types.
            auto v = value.GetInt();
            modified |= ui::DragInt("", &v, int_step, int_min, int_max);
            if (modified)
                value = v;
            break;
        }
        case VAR_BOOL:
        {
            auto v = value.GetBool();
            modified |= ui::Checkbox("", &v);
            if (modified)
                value = v;
            break;
        }
        case VAR_FLOAT:
        {
            auto v = value.GetFloat();
            modified |= ui::DragFloat("", &v, float_step, float_min, float_max, "%.3f", 3.0f);
            if (modified)
                value = v;
            break;
        }
        case VAR_VECTOR2:
        {
            auto& v = value.GetVector2();
            modified |= ui::DragFloat2("xy", const_cast<float*>(&v.x_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_VECTOR3:
        {
            auto& v = value.GetVector3();
            modified |= ui::DragFloat3("xyz", const_cast<float*>(&v.x_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_VECTOR4:
        {
            auto& v = value.GetVector4();
            modified |= ui::DragFloat4("xyzw", const_cast<float*>(&v.x_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_QUATERNION:
        {
            auto v = value.GetQuaternion().EulerAngles();
            modified |= ui::DragFloat3("xyz", const_cast<float*>(&v.x_), float_step, float_min, float_max, "%.3f", 3.0f);
            if (modified)
                value = Quaternion(v.x_, v.y_, v.z_);
            break;
        }
        case VAR_COLOR:
        {
            auto& v = value.GetColor();
            modified |= ui::ColorEdit4("rgba", const_cast<float*>(&v.r_));
            break;
        }
        case VAR_STRING:
        {
            auto& v = const_cast<String&>(value.GetString());
            auto& buffer = GetBuffer(info.name_, value.GetString());
            modified |= ui::InputText("", &buffer.front(), buffer.size() - 1);
            if (modified)
                value = &buffer.front();
            break;
        }
//            case VAR_BUFFER:
        case VAR_VOIDPTR:
            ui::Text("%p", value.GetVoidPtr());
            break;
        case VAR_RESOURCEREF:
        {
            auto ref = value.GetResourceRef();
            ui::Text("%s", ref.name_.CString());
            ui::SameLine();
            if (ui::Button(ICON_FA_FOLDER_OPEN))
            {
                auto cache = GetSubsystem<ResourceCache>();
                auto file_name = cache->GetResourceFileName(ref.name_);
                String selected_path = tinyfd_openFileDialog(
                    ToString("Open %s File", context_->GetTypeName(ref.type_).CString()).CString(),
                    file_name.Length() ? file_name.CString() : GetFileSystem()->GetCurrentDir().CString(), 0, 0, 0, 0);
                SharedPtr<Resource> resource(cache->GetResource(ref.type_, selected_path));
                if (resource.NotNull())
                {
                    ref.name_ = resource->GetName();
                    value = ref;
                    modified = true;
                }
            }
            break;
        }
//            case VAR_RESOURCEREFLIST:
//            case VAR_VARIANTVECTOR:
//            case VAR_VARIANTMAP:
        case VAR_INTRECT:
        {
            auto& v = value.GetIntRect();
            modified |= ui::DragInt4("ltbr", const_cast<int*>(&v.left_), int_step, int_min, int_max);
            break;
        }
        case VAR_INTVECTOR2:
        {
            auto& v = value.GetIntVector2();
            modified |= ui::DragInt2("xy", const_cast<int*>(&v.x_), int_step, int_min, int_max);
            break;
        }
        case VAR_PTR:
            ui::Text("%p (Void Pointer)", value.GetPtr());
            break;
        case VAR_MATRIX3:
        {
            auto& v = value.GetMatrix3();
            modified |= ui::DragFloat3("m0", const_cast<float*>(&v.m00_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat3("m1", const_cast<float*>(&v.m10_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat3("m2", const_cast<float*>(&v.m20_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_MATRIX3X4:
        {
            auto& v = value.GetMatrix3x4();
            modified |= ui::DragFloat4("m0", const_cast<float*>(&v.m00_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat4("m1", const_cast<float*>(&v.m10_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat4("m2", const_cast<float*>(&v.m20_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_MATRIX4:
        {
            auto& v = value.GetMatrix4();
            modified |= ui::DragFloat4("m0", const_cast<float*>(&v.m00_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat4("m1", const_cast<float*>(&v.m10_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat4("m2", const_cast<float*>(&v.m20_), float_step, float_min, float_max, "%.3f", 3.0f);
            modified |= ui::DragFloat4("m3", const_cast<float*>(&v.m30_), float_step, float_min, float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_DOUBLE:
        {
            // TODO: replace this with custom control that properly handles double types.
            float v = value.GetDouble();
            modified |= ui::DragFloat("", &v, float_step, float_min, float_max, "%.3f", 3.0f);
            if (modified)
                value = (double)v;
            break;
        }
        case VAR_STRINGVECTOR:
        {
            auto index = 0;
            auto& v = const_cast<StringVector&>(value.GetStringVector());

            // Insert new item.
            {
                auto& buffer = GetBuffer(info.name_, "");
                ui::PushID(index++);
                if (ui::InputText("", &buffer.front(), buffer.size() - 1, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    v.Push(&buffer.front());
                    buffer.front() = 0;
                    modified = true;
                }
                ui::PopID();
            }

            // List of current items.
            for (String& sv: v)
            {
                auto buffer_name = ToString("%s-%d", info.name_.CString(), index);
                auto& buffer = GetBuffer(buffer_name, sv);
                ui::PushID(index++);
                if (ui::Button(ICON_FA_TRASH))
                {
                    RemoveBuffer(buffer_name);
                    v.Remove(sv);
                    modified = true;
                    ui::PopID();
                    break;
                }
                ui::SameLine();

                modified |= ui::InputText("", &buffer.front(), buffer.size() - 1,
                                          ImGuiInputTextFlags_EnterReturnsTrue);
                if (modified)
                    sv = &buffer.front();
                ui::PopID();
            }

            if (modified)
                value = StringVector(v);

            break;
        }
        case VAR_RECT:
        {
            auto& v = value.GetRect();
            modified |= ui::DragFloat2("min xy", const_cast<float*>(&v.min_.x_), float_step, float_min,
                                       float_max, "%.3f", 3.0f);
            ui::SameLine();
            modified |= ui::DragFloat2("max xy", const_cast<float*>(&v.max_.x_), float_step, float_min,
                                       float_max, "%.3f", 3.0f);
            break;
        }
        case VAR_INTVECTOR3:
        {
            auto& v = value.GetIntVector3();
            modified |= ui::DragInt3("xyz", const_cast<int*>(&v.x_), int_step, int_min, int_max);
            break;
        }
        case VAR_INT64:
        {
            // TODO: replace this with custom control that properly handles int types.
            int v = value.GetInt64();
            modified |= ui::DragInt("", &v, int_step, int_min, int_max, "%d");
            if (modified)
                value = (long long)v;
            break;
        }
        default:
            ui::TextUnformatted("Unhandled attribute type.");
            break;
        }
    }
    return modified;
}

AttributeInspectorWindow::AttributeInspectorWindow(Context* context) : AttributeInspector(context)
{

}

void AttributeInspectorWindow::SetEnabled(bool enabled)
{
    if (enabled && !IsEnabled())
        SubscribeToEvent(E_SYSTEMUIFRAME, std::bind(&AttributeInspectorWindow::RenderUi, this));
    else if (!enabled && IsEnabled())
        UnsubscribeFromEvent(E_SYSTEMUIFRAME);
}

void AttributeInspectorWindow::SetSerializable(Serializable* item)
{
    currentSerializable_ = item;
}

void AttributeInspectorWindow::RenderUi()
{
    if (ui::Begin("Attribute Inspector"))
    {
        if (currentSerializable_.NotNull())
            RenderAttributes(currentSerializable_);
    }
    ui::End();
}

bool AttributeInspectorWindow::IsEnabled() const
{
    return HasSubscribedToEvent(E_SYSTEMUIFRAME);
}

}
