package com.agrihub.coolhub.data.model;

import com.google.gson.annotations.SerializedName;
import java.util.List;

/**
 * Paginates a list of TelemetryReading objects from the API.
 */
public class PaginatedTelemetry {

    @SerializedName("total")
    public int total;

    @SerializedName("items")
    public List<TelemetryReading> items;
}
