package com.agrihub.coolhub.data.model;

import com.google.gson.annotations.SerializedName;
import java.util.List;

/**
 * Paginates a list of Alert objects from the API.
 */
public class PaginatedAlerts {

    @SerializedName("total")
    public int total;

    @SerializedName("items")
    public List<Alert> items;
}
