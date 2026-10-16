using System;
using System.Runtime.InteropServices;
using Godot.NativeInterop;

namespace Godot.Bridge
{
    internal static class CSharpInstanceBridge
    {

        [UnmanagedCallersOnly]
        internal static unsafe godot_bool CallViaTrampoline(
            MethodTrampolineDelegate methodTrampoline,
            IntPtr godotObjectGCHandle, godot_variant** args, int argCount,
            godot_variant_call_error* refCallError, godot_variant* outRet)
        {
            try
            {
                object godotObject = GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (godotObject == null)
                {
                    *outRet = default;
                    (*refCallError).Error = godot_variant_call_error_error.GODOT_CALL_ERROR_CALL_ERROR_INSTANCE_IS_NULL;
                    return godot_bool.False;
                }

                *outRet = methodTrampoline(godotObject, new NativeVariantPtrArgs(args, argCount), ref *refCallError);
                return godot_bool.True;
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
                *outRet = default;
                return godot_bool.False;
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe godot_bool SetViaTrampoline(
            PropertySetterTrampolineDelegate setterTrampoline,
            IntPtr godotObjectGCHandle, godot_variant* value)
        {
            try
            {
                object godotObject = GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (godotObject == null)
                    throw new InvalidOperationException("Godot Object instance is null.");

                setterTrampoline(godotObject, *value);
                return godot_bool.True;
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
                return godot_bool.False;
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe godot_bool GetViaTrampoline(
            PropertyGetterTrampolineDelegate getterTrampoline,
            IntPtr godotObjectGCHandle, godot_variant* outRet)
        {
            try
            {
                object godotObject = GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (godotObject == null)
                    throw new InvalidOperationException("Godot Object instance is null.");

                *outRet = getterTrampoline(godotObject);
                return godot_bool.True;
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
                *outRet = default;
                return godot_bool.False;
            }
        }

        [UnmanagedCallersOnly]
        internal static void CallDispose(IntPtr godotObjectGCHandle, godot_bool okIfNull)
        {
            try
            {
                var godotObject = (GodotObject)GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (okIfNull.ToBool())
                    godotObject?.Dispose();
                else
                    godotObject!.Dispose();
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe void CallToString(IntPtr godotObjectGCHandle, godot_string* outRes, godot_bool* outValid)
        {
            try
            {
                var self = (GodotObject)GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (self == null)
                {
                    *outRes = default;
                    *outValid = godot_bool.False;
                    return;
                }

                var resultStr = self.ToString();

                if (resultStr == null)
                {
                    *outRes = default;
                    *outValid = godot_bool.False;
                    return;
                }

                *outRes = Marshaling.ConvertStringToNative(resultStr);
                *outValid = godot_bool.True;
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
                *outRes = default;
                *outValid = godot_bool.False;
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe void SerializeState(
            IntPtr godotObjectGCHandle,
            godot_dictionary* propertiesState,
            godot_dictionary* signalEventsState
        )
        {
            try
            {
                var godotObject = (GodotObject)GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (godotObject == null)
                    return;

                // Call OnBeforeSerialize

                // ReSharper disable once SuspiciousTypeConversion.Global
                if (godotObject is ISerializationListener serializationListener)
                    serializationListener.OnBeforeSerialize();

                // Save instance state

                using var info = GodotSerializationInfo.CreateCopyingBorrowed(
                    *propertiesState, *signalEventsState);

                godotObject.SaveGodotObjectData(info);
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
            }
        }

        [UnmanagedCallersOnly]
        internal static unsafe void DeserializeState(
            IntPtr godotObjectGCHandle,
            godot_dictionary* propertiesState,
            godot_dictionary* signalEventsState
        )
        {
            try
            {
                var godotObject = (GodotObject)GCHandle.FromIntPtr(godotObjectGCHandle).Target;

                if (godotObject == null)
                    return;

                // Restore instance state

                using var info = GodotSerializationInfo.CreateCopyingBorrowed(
                    *propertiesState, *signalEventsState);

                godotObject.RestoreGodotObjectData(info);

                // Call OnAfterDeserialize

                // ReSharper disable once SuspiciousTypeConversion.Global
                if (godotObject is ISerializationListener serializationListener)
                    serializationListener.OnAfterDeserialize();
            }
            catch (Exception e)
            {
                ExceptionUtils.LogException(e);
            }
        }
    }
}
