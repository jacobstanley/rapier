Linux Host Pricing
==================

_Last Updated 2020-Jul-25_

## AWS EC2 Spot

I suspect Zorro+MT4 might actually run fine on an `m1.small` but
I decided on the `m3.medium` as it was slightly more pleasant to work
with the beefier box.

Prices are for spot instances in USD.

Note that with spot instances the cost tradeoff is that you could have
your instance terminated at any time so you may need to be able to deal
with that. This doesn't happen very often (if ever) assuming you bid
a high enough price.

With all of these instances you need to pay fees for the EBS volume that
is mounted to them, that will be USD 0.10/GB per month on top of the
cost below. I am using a 10 GB volume so that's an extra USD 1.00/month.

| type | memory (GiB) | region | USD/hour | USD/month | USD/year |
|---|---|---|---|---|---|
| m1.small  | 1.7  | us-east-1       | 0.0044 | 3.21 | 38.54 |
| m1.small  | 1.7  | us-west-1       | 0.0047 | 3.43 | 41.17 |
| m1.small  | 1.7  | us-southeast-1  | 0.0058 | 4.23 | 50.81 |
| m1.small  | 1.7  | us-southeast-2  | 0.0058 | 4.23 | 50.81 |
| m3.medium | 3.75 | us-east-1       | 0.0067 | 4.89 | 58.69 |
| m3.medium | 3.75 | us-west-1       | 0.0077 | 5.62 | 67.45 |
| m3.medium | 3.75 | us-southeast-1  | 0.0098 | 7.15 | 85.85 |
| m3.medium | 3.75 | us-southeast-2  | 0.0093 | 6.79 | 81.47 |

_month = 730 hours, year = 8760 hours_

T2/3 instances probably can't be used as they are for burstable
workloads and I kept running out of CPU credits running a constant
workload like Zorro. Maybe this wouldn't be an issue if I could fix the
100% CPU issue with `winedevice.exe`.

| region | memory (GiB) | type | USD/hour | USD/month | USD/year |
|---|---|---|---|---|---|
| t2.micro  | 1.0 | us-southeast-2  | 0.0044 | 3.21  | 38.54 |
| t3.nano   | 0.5 | us-east-1       | 0.0016 | 1.17  | 14.02 |
| t3.micro  | 1.0 | us-east-1       | 0.0031 | 2.26  | 27.16 |
| t3.micro  | 1.0 | us-west-1       | 0.0037 | 2.70  | 32.41 |
| t3.micro  | 1.0 | us-southeast-1  | 0.0040 | 2.92  | 35.04 |
| t3.micro  | 1.0 | us-southeast-2  | 0.0040 | 2.92  | 35.04 |
| t3a.micro | 1.0 | us-southeast-2  | 0.0036 | 2.63  | 31.54 |
| t3.small  | 2.0 | us-east-1       | 0.0062 | 4.53  | 54.31 |
| t3.small  | 2.0 | us-southeast-2  | 0.0079 | 5.77  | 69.20 |
| t3a.small | 2.0 | us-southeast-2  | 0.0071 | 5.18  | 62.20 |
| t3.medium | 4.0 | us-southeast-2  | 0.0158 | 11.53 | 138.41 |

### Region Locations

Quick ref if you don't use AWS all the time. There are many more regions
but these are the ones that I collected spot prices for.

| region | location |
|---|---|
| us-east1 | North Virginia |
| us-west1 | North California |
| ap-southeast1 | Singapore |
| ap-southeast2 | Sydney |


## Digital Ocean

Prices are the same in every region.

| memory | USD/month |
|---|---|
| 1 GiB |  5.00 |
| 2 GiB | 10.00 |
| 3 GiB | 15.00 |

_note: for comparison EC2 m3.medium memory = 3.75 GiB_

## Others

_todo: list the prices / specs in a table here_

- https://contabo.com/?show=vps
- https://www.routerhosting.com/windows-vps/
- https://servarica.com/nvme-servers/
- https://clouding.io/en/
