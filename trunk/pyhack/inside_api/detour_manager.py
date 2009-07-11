from detour_callback_object import DetourCallbackObject

class _DetourManager(dict):
    """DetourManager is a class that managers a group of detours. It's keys are addresses, and it's values are Detour instances"""

    def do_callback(self, address, registers, caller):
        """
            This function gets slightly massaged arguments from the initial dispatcher.
            It's job is to call the function set as the callback for this particular detour.
        """
        if address not in self:
            raise Exception, "Detour... doesn't exist...?"
            
        detour = self[address]
            
        if callable(detour.config.callback):
            obj = None
            try:
                obj = detour.config.callback_class(detour, registers, caller)
                if obj is None:
                    obj = DetourCallbackObject(detour, registers, caller)
                detour.config.callback(obj)
            except Exception, e:
                import traceback
                print "Exception in callback for function at address 0x%08x:\n"%(address)
                traceback.print_exc()
                print ""
                #raise e
            try:
                obj.applyRegisters()
            except LookupError: #could have removed the detour from inside the callback function
                pass

DetourManager = _DetourManager()