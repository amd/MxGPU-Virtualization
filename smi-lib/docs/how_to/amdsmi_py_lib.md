---
myst:
  html_meta:
    "description lang=en": "Get started with the AMD SMI Python interface."
    "keywords": "api, smi, lib, py, system, management, interface"
---

# Python package usage

Before creating python package make sure that python version on your sistem is at least 3.10

Run ```make package``` to create the AMD SMI Python package.
Open terminal and navigate to ```gim/smi-lib/build/amdsmi/package/Release```.
Run ```sudo python3``` command. This command will launch the Python interpreter, allowing you to execute Python code and interact with the Python environment.
First, you will need to execute the following lines of code:

```python
from amdsmi import * # imports all the functions and classes defined in the amdsmi package, allowing you to use them directly without specifying the package name.
amdsmi_init() # function is then called to initialize the AMD System Management Interface (SMI) library. This function sets up the necessary environment and
# resources for interacting with the AMD GPU and accessing its management information.
```

By executing these lines, you can access the functionality provided by the amdsmi package and start using the AMD SMI library.
Most functions require you to specify the processor handle for which you want to retrieve information. In order to do that, it is necessary to call amdsmi_get_processor_handles() function, which will return the list of GPU device handle objects on the current machine. Executing len(processors) function, will be able to see the number of elements of the mentioned list:

```python
processors = amdsmi_get_processor_handles()
len(processors)
```

Once you have obtained the processor handle, you can call any API that works with it. For example:

```python
 amdsmi_get_gpu_asic_info(processors[0])
```

Finally, call amdsmi_shut_down() to release the SMI library resources and close the connection to the driver:

```python
amdsmi_shut_down()
```

```python
from amdsmi import *

try:
    amdsmi_init()

    # amdsmi calls ...

except AmdSmiException as e:
    print(e)
finally:
    try:
        amdsmi_shut_down()
    except AmdSmiException as e:
        print(e)
```
