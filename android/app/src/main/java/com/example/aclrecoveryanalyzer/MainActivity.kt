package com.example.aclrecoveryanalyzer

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.example.aclrecoveryanalyzer.ui.navigation.AppNavHost
import com.example.aclrecoveryanalyzer.ui.theme.AclRehabTheme
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AclRehabTheme {
                AppNavHost()
            }
        }
    }
}