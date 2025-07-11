# Event Monitor Example

This example reads events from the GPU with configurable category and severity filtering.

## Description

The program accepts two optional arguments to filter GPU events:
- **Category**: Event category (0-14) representing `AMDSMI_EVENT_CATEGORY`
- **Severity**: Severity level (0-4) where lower numbers indicate higher severity

If no arguments are provided, all GPU events are displayed. If the category value is 15 or higher, events from all categories are included, but they are filtered based on the specified severity level.

## Dependencies

- `libamdsmi.so`

## Prerequisites

- OS dynamic library search path must include the location of `libamdsmi.so`. For example, you can set the `LD_LIBRARY_PATH` as follows:
  ```bash
  export LD_LIBRARY_PATH=/path/to/libamdsmi/library:$LD_LIBRARY_PATH
  ```

event_monitor <category> <severity>

Here, `<category>` specifies the GPU event category (0-15), and `<severity>` defines the severity level (0-4) for filtering events.

```bash
event_monitor <category> <severity>
```

### Arguments

#### Category (0-14)
| Value | Category |
|-------|----------|
| 0 | `AMDSMI_EVENT_CATEGORY_NON_USED` |
| 1 | `AMDSMI_EVENT_CATEGORY_DRIVER` |
| 2 | `AMDSMI_EVENT_CATEGORY_RESET` |
| 3 | `AMDSMI_EVENT_CATEGORY_SCHED` |
| 4 | `AMDSMI_EVENT_CATEGORY_VBIOS` |
| 5 | `AMDSMI_EVENT_CATEGORY_ECC` |
| 6 | `AMDSMI_EVENT_CATEGORY_PP` |
| 7 | `AMDSMI_EVENT_CATEGORY_IOV` |
| 8 | `AMDSMI_EVENT_CATEGORY_VF` |
| 9 | `AMDSMI_EVENT_CATEGORY_FW` |
| 10 | `AMDSMI_EVENT_CATEGORY_GPU` |
| 11 | `AMDSMI_EVENT_CATEGORY_GUARD` |
| 12 | `AMDSMI_EVENT_CATEGORY_GPUMON` |
| 13 | `AMDSMI_EVENT_CATEGORY_MMSCH` |
| 14 | `AMDSMI_EVENT_CATEGORY_XGMI` |
| ≥15 | All categories |

#### Severity Level (0-4)
| Value | Level | Description |
|-------|-------|-------------|
| 0 | Critical | Highest severity, critical errors |
| 1 | Medium | Significant errors |
| 2 | Low | Trivial errors |
| 3 | Warning | Warning level |
| 4 | Info | All events (informational) |

> **Warning:** Invalid severity values default to the lowest severity level (2), which corresponds to "Low" as per the table above.

## Examples

### System-installed library
```bash
sudo ./event_monitor 2 1
```

### Custom library path
```bash
sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./event_monitor 2 1
```