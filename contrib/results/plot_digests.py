#!/usr/bin/env python3

import argparse
import matplotlib.pyplot as plt
from matplotlib.collections import EventCollection
import requests

# Bound graphs to preserve inner detail
# We put the max/min in a text box so the outliers are
# still available
q_graph_min = 0.005
q_graph_max = 0.995
q_graph_scalar = 1000

def lerp(a, b, t):
    #print('lerp({}, {}, {})'.format(a, b, t))
    if (a <= 0 and b >= 0) or (a >= 0 and b <= 0):
        return t * b + (1 - t) * a

    if t == 1:
        return b

    x = a + t * (b - a)
    if (t > 1) == (b > a):
        return x if b < x else b
    else:
        return x if x < b else b


def mean(obj):
    return obj['mean']


def weight(obj):
    return obj['weight']

# Minimal t-digest implementation. It's really just a wrapper for the REST
# API's centroid output that allows us to calculate cdfs and quantiles.
class digest:
    def __init__(self, centroids, min_val, max_val):
        self.min_val = min_val
        self.max_val = max_val
        self.centroids = centroids
        self.total_weight = sum([weight(obj) for obj in centroids])

    @property
    def min(self):
        return self.min_val

    @property
    def max(self):
        return self.max_val

    def cdf(self, m):

        if m <= self.min_val:
            return float(0)

        if m >= self.max_val:
            return float(1)

        front = self.centroids[0]
        if m < mean(front): # interpolate between min and first mean
            x = (m - self.min_val) / (mean(front) - self.min_val)
            return lerp(0, max(weight(front) / 2, 1), x) / self.total_weight

        back = self.centroids[-1]
        if m > mean(back): # interpolate between last mean and max
            x = (m - mean(back)) / (self.max_val - mean(back))
            left = self.total_weight - max(weight(back) / 2, 1)
            return lerp(left, self.total_weight, x) / self.total_weight

        t = weight(front) / 2

        for i in range(len(self.centroids) - 1):
            current = self.centroids[i]
            next = self.centroids[i + 1]
            if mean(current) == m: # sum up all matching centroid values
                idx = i
                delta_w = 0
                while mean(current) == m:
                    delta_w += weight(current)
                    idx += 1
                    current = self.centroids[idx]
                return (t + delta_w / 2) / self.total_weight
            else:
                delta_w = (weight(current) + weight(next)) / 2
                if mean(current) < m and m < mean(next):
                    x = (m - mean(current)) / (mean(next) - mean(current))
                    return (t + lerp(0, delta_w, x)) / self.total_weight

                t += delta_w

        return (1.0)

    def quantile(self, p):
        if p < 0 or p > 1:
            raise ValueError("Invalid percentile value")

        q = p * self.total_weight

        if q < 1:
            return self.min_val

        front = self.centroids[0]
        if weight(front) > 1 and q < weight(front) / 2:
            x = (q - 1) / (weight(front) / 2 - 1)
            return lerp(self.min_val, mean(front), x)

        if q > self.total_weight - 1:
            return self.max_val

        back = self.centroids[-1]
        if weight(back) > 1 and self.total_weight - q <= weight(back) / 2:
            x = (self.total_weight - q - 1) / (weight(back) / 2 - 1)
            return lerp(mean(back), self.max_val, x)

        t = weight(front) / 2

        for i in range(len(self.centroids) - 1):
            current = self.centroids[i]
            next = self.centroids[i + 1]
            delta_w = (weight(current) + weight(next)) / 2
            if q < t + delta_w:
                x = (q - t) / delta_w
                return lerp(mean(current), mean(next), x)

            t += delta_w

        # max was checked above; interpolate between q and max
        x = (q - self.total_weight - weight(back) / 2) / (weight(back) / 2)
        return lerp(mean(back), self.max_val, x)


def to_text_string(summary, frame_count):
    return '\n'.join([
        r'$\mathrm{{min}}={}$'.format(summary['min']),
        r'$\mathrm{{mean}}={}$'.format(int(summary['total'] / frame_count)),
        r'$\mathrm{{max}}={}$'.format(summary['max']),
        r'$\sigma={}$'.format(summary['std_dev'])
    ])


def to_pretty_name(tag):
    names = {
        'interarrival': 'Interarrival Time',
        'jitter_ipdv': 'Jitter (IPDV)',
        'jitter_rfc': 'Jitter (RFC)'
    }

    return names[tag] if tag in names else tag.capitalize()


def get_midpoint(digest):
    return (digest.quantile(q_graph_min)
            + (digest.quantile(q_graph_max) - digest.quantile(q_graph_min)) / 2)


def generate_matplotlib_cdf(uid, tag, units, digest, summary, frame_count):
    # Generate x/y points
    x = []
    y = []
    left = digest.quantile(q_graph_min)
    right = digest.quantile(q_graph_max)
    step = (right - left) * 0.01
    for w in range(int(left), int(right + step), int(step)):
        x.append(w)
        y.append(digest.cdf(w))

    xevents = [mean(c) for c in digest.centroids]


    # Draw the plot
    fig = plt.figure()
    ax = fig.add_subplot()

    ax.set_title('{} CDF'.format(to_pretty_name(tag)))
    ax.set_ylabel('Cumulative Density')
    ax.set_ylim([0, 1])
    ax.set_xlabel('{} - {}'.format(to_pretty_name(tag), units))

    ax.plot(x, y)

    ax.add_collection(EventCollection(xevents, linelength=0.05))

    # Use the graph to determine text placement
    if digest.quantile(.5) < get_midpoint(digest):
        x_txt, y_txt = 0.65, 0.07
    else:
        x_txt, y_txt = 0.05, 0.7

    ax.text(x_txt, y_txt,
            to_text_string(summary, frame_count),
            transform=ax.transAxes,
            fontsize=14,
            verticalalignment='bottom',
            horizontalalignment='left',
            bbox=dict(boxstyle='round',
                      facecolor='aliceblue',
                      alpha=0.5))

    fig.savefig('{}-{}-cdf.png'.format(uid, tag))


def generate_matplotlib_pdf(uid, tag, units, digest, summary, frame_count):
    # Generate x/y points
    x = []
    y = []
    left = q_graph_min * q_graph_scalar
    right = q_graph_max * q_graph_scalar
    step = (right - left) * 0.01
    for q in range(int(left), int(right + step), int(step)):
        pct = q / q_graph_scalar
        x.append(pct)
        y.append(digest.quantile(pct))

    # Generate x event points
    xevents = []
    t = 0
    for c in digest.centroids:
        t += weight(c)
        xevents.append(t / digest.total_weight)

    # Draw the plot
    fig = plt.figure()
    ax = fig.add_subplot()

    ax.set_title('{} PDF'.format(to_pretty_name(tag)))
    ax.set_xlabel('Probability Distribution')
    ax.set_xlim([0, 1])
    ax.set_ylabel('{} - {}'.format(to_pretty_name(tag), units))

    ax.plot(x, y)

    event_length = (y[-1] - y[0]) / 20
    ax.add_collection(EventCollection(xevents,
                                      lineoffset=y[0] - event_length,
                                      linelength=event_length))

    # Use the graph to determine text placement
    if digest.quantile(.5) > get_midpoint(digest):
        x_txt, y_txt = 0.65, 0.07
    else:
        x_txt, y_txt = 0.05, 0.7

    ax.text(x_txt, y_txt,
            to_text_string(summary, frame_count),
            transform=ax.transAxes,
            fontsize=14,
            verticalalignment='bottom',
            horizontalalignment='left',
            bbox=dict(boxstyle='round',
                      facecolor='aliceblue',
                      alpha=0.5))

    fig.savefig('{}-{}-pdf.png'.format(uid, tag))


def parse_digest(uid, tag, units, centroids, summary, frame_count):
    min_val = summary['min']
    max_val = summary['max']
    d = digest(centroids, min_val, max_val)

    avg = summary['total'] / frame_count
    generate_matplotlib_cdf(uid, tag, units, d, summary, frame_count)
    generate_matplotlib_pdf(uid, tag, units, d, summary, frame_count)


def handle_result(result, counters_key, digests_key):
    frame_count = result[counters_key]['frame_count']
    uid = result['id']

    if digests_key in result:
        digests = result[digests_key]
        if 'interarrival' in digests:
            centroids = digests['interarrival']['centroids']
            summary = result[counters_key]['interarrival']['summary']
            units = result[counters_key]['interarrival']['units']
            parse_digest(uid, 'interarrival', units, centroids, summary, frame_count)

        if 'latency' in digests:
            centroids = digests['latency']['centroids']
            summary = result[counters_key]['latency']['summary']
            units = result[counters_key]['latency']['units']
            parse_digest(uid, 'latency', units, centroids, summary, frame_count)

        if 'jitter_ipdv' in digests:
            centroids = digests['jitter_ipdv']['centroids']
            summary = result[counters_key]['jitter_ipdv']['summary']
            units = result[counters_key]['jitter_ipdv']['units']
            parse_digest(uid, 'jitter_ipdv', units, centroids, summary, frame_count)

        if 'jitter_rfc' in digests:
            centroids = digests['jitter_rfc']['centroids']
            summary = result[counters_key]['jitter_rfc']['summary']
            units = result[counters_key]['jitter_rfc']['units']
            parse_digest(uid, 'jitter_rfc', units, centroids, summary, frame_count)


def handle_analyzer_result(url, uid):
    get = requests.get("/".join([url, 'packet', 'analyzer-results', uid]))
    get.raise_for_status()
    handle_result(get.json(), 'flow_counters', 'flow_digests')


def handle_analyzer_results(url):
    get = requests.get("/".join([url, 'packet', 'analyzer-results']))
    get.raise_for_status()
    for obj in get.json():
        handle_result(obj, 'flow_counters', 'flow_digests')


def handle_flow_result(url, uid):
    get = requests.get("/".join([url, 'packet', 'rx-flows', uid]))
    get.raise_for_status()
    handle_result(get.json(), 'counters', 'digests')


def handle_flow_results(url):
    get = requests.get("/".join([url, 'packet', 'rx-flows']))
    get.raise_for_status()
    for obj in get.json():
        handle_result(obj, 'counters', 'digests')


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Turn OpenPerf digest data into Cumulative Distribution Function (CDF) '
            + 'and Proability Density Function (PDF) graphs. Files are written to '
            + 'the current working directory.'))
    parser.add_argument('url', help='OpenPerf REST API root URL')
    parser.add_argument('source', choices=['analyzer-results', 'rx-flows'],
                        help="digest data source")
    parser.add_argument('--id', type=str,
                        help="specify a specific result id to query")
    args = parser.parse_args()

    if args.source == 'analyzer-results':
        if args.id is not None:
            handle_analyzer_result(args.url, args.id)
        else:
            handle_analyzer_results(args.url)
    else:
        if args.id is not None:
            handle_flow_result(args.url, args.id)
        else:
            handle_flow_results(args.url)


if __name__ == "__main__":
    main()
